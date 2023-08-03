#include <UnAsync/Jobs/Job.h>
#include <UnAsync/Pipes/Pipe.h>

namespace UN::Async
{
    Pipe::Pipe(const PipeDesc& desc)
        : m_Desc(desc)
        , m_WriterAwaitable(true)
    {
        UN_Assert(m_Desc.JobScheduler, "Job scheduler was not set in the descriptor");

        if (!m_Desc.Pool)
        {
            m_Desc.Pool = AllocateObject<ArrayPool<Byte>>(SystemAllocator::Get());
        }

        m_SegmentPool.Reserve(m_Desc.InitialSegmentPoolSize);
    }

    void Pipe::RentMemory(Pipe::BufferSegment* segment, USize sizeHint)
    {
        UN_Assert(segment->Capacity() == 0, "Segment must be empty");
        UN_Assert(sizeHint >= 0, "Size hist must be non-negative");

        auto size   = std::max(m_Desc.MinimumSegmentSize, sizeHint);
        auto memory = m_Desc.Pool->Rent(size);
        segment->SetMemory(memory);
        m_WritingHeadMemory = segment->GetAvailableMemory();
    }

    Pipe::BufferSegment* Pipe::AllocateSegment(USize sizeHint)
    {
        BufferSegment* newSegment;
        if (m_SegmentPool.Any())
        {
            newSegment = m_SegmentPool.Pop();
        }
        else
        {
            // TODO: store segments in a memory pool
            newSegment = new BufferSegment;
        }

        RentMemory(newSegment, sizeHint);
        return newSegment;
    }

    void Pipe::ReturnSegment(Pipe::BufferSegment* pSegment)
    {
        UN_Assert(pSegment != m_pWritingHead, "Returning a segment that is still in use");
        UN_Assert(pSegment != m_pReadingTail, "Returning a segment that is still in use");
        UN_Assert(pSegment != m_pReadingHead, "Returning a segment that is still in use");

        if (m_SegmentPool.Size() < m_Desc.MaximumSegmentPoolSize)
        {
            m_SegmentPool.Push(pSegment);
        }
        else
        {
            delete pSegment;
        }
    }

    void Pipe::AllocateWritingHead(USize sizeHint)
    {
        if (!AllFlagsActive(m_OperationState, State::Writing) || m_WritingHeadMemory.Length() == 0
            || m_WritingHeadMemory.Length() < sizeHint)
        {
            AllocateWritingHeadSync(sizeHint);
        }
    }

    void Pipe::AdvanceUnsynchronized(USize byteCount)
    {
        m_BytesNotFlushed += byteCount;
        m_WritingHeadBytesBuffered += byteCount;
        m_WritingHeadMemory = byteCount == m_WritingHeadMemory.Length()
            ? ArraySlice<Byte>{}
            : m_WritingHeadMemory(byteCount, m_WritingHeadMemory.Length());
    }

    void Pipe::AllocateWritingHeadSync(USize sizeHint)
    {
        std::unique_lock lk(m_Mutex);

        m_OperationState |= State::Writing;

        if (m_pWritingHead == nullptr)
        {
            m_pWritingHead = m_pReadingHead = m_pReadingTail = AllocateSegment(sizeHint);

            m_LastExaminedIndex = 0;
            return;
        }

        auto bytesLeftInBuffer = m_WritingHeadMemory.Length();

        if (bytesLeftInBuffer == 0 || bytesLeftInBuffer < sizeHint)
        {
            if (m_WritingHeadBytesBuffered > 0)
            {
                m_pWritingHead->AdvanceEnd(m_WritingHeadBytesBuffered);
                m_WritingHeadBytesBuffered = 0;
            }

            if (m_pWritingHead->End() == 0)
            {
                m_Desc.Pool->Return(m_pWritingHead->GetAvailableMemory());
                RentMemory(m_pWritingHead, sizeHint);
            }
            else
            {
                auto* newSegment = AllocateSegment(sizeHint);
                m_pWritingHead->SetNext(newSegment);
                m_pWritingHead = newSegment;
            }
        }
    }

    bool Pipe::CommitUnsynchronized()
    {
        RemoveFlags(m_OperationState, State::Writing);

        if (m_BytesNotFlushed == 0)
        {
            return false;
        }

        m_pWritingHead->AdvanceEnd(m_WritingHeadBytesBuffered);

        m_pReadingTail     = m_pWritingHead;
        m_ReadingTailIndex = m_pWritingHead->End();

        auto oldLength = m_BytesNotConsumed;
        m_BytesNotConsumed += m_BytesNotFlushed;

        auto resumeReader = true;
        if (m_BytesNotConsumed < m_MinimumReadBytes)
        {
            resumeReader = false;
        }
        else if (m_Desc.PauseWriterThreshold > 0 && oldLength < m_Desc.PauseWriterThreshold
                 && m_BytesNotConsumed >= m_Desc.PauseWriterThreshold && !m_ReaderComplete)
        {
            m_WriterAwaitable.Reset();
        }

        m_BytesNotFlushed          = 0;
        m_WritingHeadBytesBuffered = 0;

        return resumeReader;
    }

    void Pipe::Schedule(AsyncEvent& event)
    {
        // Run the awaitable continuation on a job scheduler thread without awaiting it.
        Ptr self = this;
        Job::RunOneTime(m_Desc.JobScheduler.Get(), [self, &event]() {
            [[likely]] if (!self->m_WriterComplete || !self->m_ReaderComplete)
            {
                event.Set();
            }
        });
    }

    Task<PipeFlushResult> Pipe::FlushAsync(const CancellationToken& cancellationToken)
    {
        if (cancellationToken.IsCancellationRequested())
        {
            co_return PipeFlushResult(PipeResultFlags::Cancelled);
        }

        m_Mutex.lock();
        auto completeReader = CommitUnsynchronized();

        if (!m_WriterAwaitable.IsSet())
        {
            m_Mutex.unlock();
            co_await m_WriterAwaitable;
            m_Mutex.lock();
        }

        auto result = PipeResultFlags::None;
        if (m_ReaderComplete)
        {
            result |= PipeResultFlags::Completed;
        }
        if (cancellationToken.IsCancellationRequested())
        {
            result |= PipeResultFlags::Cancelled;
        }

        m_Mutex.unlock();

        if (completeReader)
        {
            Schedule(m_ReaderAwaitable);
        }

        co_return PipeFlushResult(result);
    }

    void Pipe::CompleteWriter()
    {
        bool readerCompleted;
        {
            std::unique_lock lk(m_Mutex);

            CommitUnsynchronized();
            m_WriterComplete = true;
            readerCompleted  = m_ReaderComplete;
        }

        if (readerCompleted)
        {
            CompletePipe();
        }

        Schedule(m_ReaderAwaitable);
    }

    void Pipe::CompleteReader()
    {
        bool writerCompleted;
        {
            std::unique_lock lk(m_Mutex);

            RemoveFlags(m_OperationState, State::AllReading);
            m_ReaderComplete = true;
            writerCompleted  = m_WriterComplete;
        }

        if (writerCompleted)
        {
            CompletePipe();
        }

        Schedule(m_WriterAwaitable);
    }

    void Pipe::CompletePipe()
    {
        std::unique_lock lk(m_Mutex);

        auto* segment = m_pReadingHead == nullptr ? m_pReadingTail : m_pReadingHead;
        while (segment != nullptr)
        {
            auto* returnSegment = segment;
            segment             = segment->Next();
            delete returnSegment;
        }

        for (auto* s : m_SegmentPool)
        {
            delete s;
        }

        m_SegmentPool.Clear();

        m_pReadingHead = m_pReadingTail = m_pWritingHead = nullptr;

        m_LastExaminedIndex = static_cast<USize>(-1);
        m_WritingHeadMemory = {};
    }

    void Pipe::AdvanceReader(const SequencePosition<Byte>& consumed, const SequencePosition<Byte>& examined)
    {
        auto scheduleWriter = false;
        {
            std::unique_lock lk(m_Mutex);
            auto examinedEverything = false;
            if (examined.Segment() == m_pReadingTail)
            {
                examinedEverything = examined.Index() == m_ReadingTailIndex;
            }

            if (examined.Segment() != nullptr && m_LastExaminedIndex != static_cast<USize>(-1))
            {
                USize examinedBytes = examined - m_LastExaminedIndex;
                USize oldLength     = m_BytesNotConsumed;

                UN_Assert(examinedBytes >= 0, "Invalid examined position");

                m_BytesNotConsumed -= examinedBytes;
                m_LastExaminedIndex = examined.Segment()->RunningIndex() + examined.Index();

                UN_Assert(m_BytesNotConsumed >= 0, "Length was negative");

                if (oldLength >= m_Desc.ResumeWriterThreshold && m_BytesNotConsumed < m_Desc.ResumeWriterThreshold)
                {
                    scheduleWriter = true;
                }
            }

            BufferSegment* returnStart = nullptr;
            BufferSegment* returnEnd   = nullptr;

            if (consumed.Segment() != nullptr)
            {
                UN_Assert(m_pReadingHead, "");

                returnStart = m_pReadingHead;
                returnEnd   = static_cast<BufferSegment*>(consumed.Segment());

                auto MoveReturnEndToNextBlock = [&]() {
                    auto* nextBlock = returnEnd->Next();
                    if (m_pReadingTail == returnEnd)
                    {
                        m_pReadingTail     = nextBlock;
                        m_ReadingTailIndex = 0;
                    }

                    m_pReadingHead     = nextBlock;
                    m_ReadingHeadIndex = 0;

                    returnEnd = nextBlock;
                };

                if (consumed.Index() == returnEnd->Length())
                {
                    if (m_pWritingHead != returnEnd)
                    {
                        MoveReturnEndToNextBlock();
                    }
                    else if (m_WritingHeadBytesBuffered == 0 && !AllFlagsActive(m_OperationState, State::Writing))
                    {
                        m_pWritingHead      = nullptr;
                        m_WritingHeadMemory = {};
                        MoveReturnEndToNextBlock();
                    }
                    else
                    {
                        m_pReadingHead     = static_cast<BufferSegment*>(consumed.Segment());
                        m_ReadingHeadIndex = consumed.Index();
                    }
                }
                else
                {
                    m_pReadingHead     = static_cast<BufferSegment*>(consumed.Segment());
                    m_ReadingHeadIndex = consumed.Index();
                }
            }

            if (examinedEverything && !m_WriterComplete)
            {
                m_ReaderAwaitable.Reset();
            }

            while (returnStart != nullptr && returnStart != returnEnd)
            {
                BufferSegment* next = returnStart->Next();
                m_Desc.Pool->Return(returnStart->GetAvailableMemory());
                returnStart->Reset();
                ReturnSegment(returnStart);
                returnStart = next;
            }

            RemoveFlags(m_OperationState, State::AllReading);
        }

        if (scheduleWriter)
        {
            Schedule(m_WriterAwaitable);
        }
    }

    Task<PipeReadResult> Pipe::ReadAsync(const CancellationToken& cancellationToken)
    {
        UN_Assert(!m_ReaderComplete, "Reader completed");
        if (cancellationToken.IsCancellationRequested())
        {
            co_return PipeReadResult(PipeResultFlags::Cancelled, {});
        }

        co_await m_ReaderAwaitable;

        {
            std::unique_lock lk(m_Mutex);

            auto flags = PipeResultFlags::None;
            if (m_WriterComplete)
            {
                flags |= PipeResultFlags::Completed;
            }
            if (cancellationToken.IsCancellationRequested())
            {
                flags |= PipeResultFlags::Cancelled;
            }

            if (m_pReadingHead)
            {
                auto begin    = SequencePosition<Byte>(m_pReadingHead, m_ReadingHeadIndex);
                auto end      = SequencePosition<Byte>(m_pReadingTail, m_ReadingTailIndex);
                auto sequence = ReadOnlySequence<Byte>(begin, end);
                co_return PipeReadResult(flags, sequence);
            }
            else
            {
                co_return PipeReadResult(flags, {});
            }
        }
    }
} // namespace UN::Async
