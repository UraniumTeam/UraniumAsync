#pragma once
#include <UnAsync/AsyncEvent.h>
#include <UnAsync/Buffers/ReadOnlySequence.h>
#include <UnAsync/Cancellation/CancellationToken.h>
#include <UnAsync/Jobs/IJobScheduler.h>
#include <UnAsync/Parallel/SpinMutex.h>
#include <UnAsync/Pipes/Internal/BufferSegment.h>
#include <UnAsync/Pipes/PipeResults.h>
#include <UnAsync/Task.h>
#include <UnTL/Base/Byte.h>
#include <UnTL/Buffers/ArrayPool.h>
#include <UnTL/Containers/ArraySlice.h>
#include <mutex>

namespace UN::Async
{
    namespace Internal
    {
        enum class PipeState : UInt8
        {
            None             = 0,
            Reading          = UN_BIT(0),
            ReadingTentative = UN_BIT(1),
            Writing          = UN_BIT(2),
            AllReading       = Reading | ReadingTentative
        };

        UN_ENUM_OPERATORS(PipeState);
    } // namespace Internal

    class PipeDesc
    {
        inline static constexpr USize DefaultPauseWriterThreshold = 65536;

    public:
        USize MinimumSegmentSize     = 4096;
        USize MaximumSegmentPoolSize = 256;
        USize InitialSegmentPoolSize = 4;
        USize PauseWriterThreshold   = DefaultPauseWriterThreshold;
        USize ResumeWriterThreshold  = DefaultPauseWriterThreshold / 2;
        Ptr<ArrayPool<Byte, SpinMutex>> Pool;
        Ptr<IJobScheduler> JobScheduler;
    };

    class Pipe final : public Object<IObject>
    {
        using State         = Internal::PipeState;
        using BufferSegment = Internal::BufferSegment;

        List<BufferSegment*> m_SegmentPool;

        PipeDesc m_Desc;
        std::mutex m_Mutex;

        USize m_BytesNotConsumed = 0;
        USize m_BytesNotFlushed  = 0;

        AsyncEvent m_WriterAwaitable;
        AsyncEvent m_ReaderAwaitable;

        bool m_WriterComplete = false;
        bool m_ReaderComplete = false;

        USize m_LastExaminedIndex = 0;

        BufferSegment* m_pReadingHead = nullptr;
        USize m_ReadingHeadIndex      = 0;

        BufferSegment* m_pReadingTail = nullptr;
        USize m_ReadingTailIndex      = 0;
        USize m_MinimumReadBytes      = 0;

        ArraySlice<Byte> m_WritingHeadMemory;
        BufferSegment* m_pWritingHead    = nullptr;
        USize m_WritingHeadBytesBuffered = 0;

        State m_OperationState = State::None;

        inline void RentMemory(BufferSegment* segment, USize sizeHint);
        inline BufferSegment* AllocateSegment(USize sizeHint);
        inline void ReturnSegment(BufferSegment* pSegment);
        inline void Schedule(AsyncEvent& event);

        void CompletePipe();
        void AllocateWritingHeadSync(USize sizeHint);
        void AllocateWritingHead(USize sizeHint);
        void AdvanceUnsynchronized(USize byteCount);
        bool CommitUnsynchronized();

    public:
        UN_RTTI_Class(Pipe, "6AF49E1B-DD81-4ED3-8D17-C1C15E68399D");

        explicit Pipe(const PipeDesc& desc);

        [[nodiscard]] inline ArraySlice<Byte> GetMemory(USize sizeHint)
        {
            UN_Assert(!m_WriterComplete, "Writer completed");

            AllocateWritingHead(sizeHint);
            return m_WritingHeadMemory;
        }

        inline void Advance(USize byteCount)
        {
            std::unique_lock lk(m_Mutex);
            UN_Assert(byteCount <= m_WritingHeadMemory.Length(), "Out of range");

            if (m_ReaderComplete)
            {
                return;
            }

            AdvanceUnsynchronized(byteCount);
        }

        Task<PipeFlushResult> FlushAsync(const CancellationToken& cancellationToken);

        void CompleteWriter();
        void CompleteReader();

        inline void AdvanceReader(const SequencePosition<Byte>& consumed)
        {
            AdvanceReader(consumed, consumed);
        }

        void AdvanceReader(const SequencePosition<Byte>& consumed, const SequencePosition<Byte>& examined);

        Task<PipeReadResult> ReadAsync(const CancellationToken& cancellationToken);
    };
} // namespace UN::Async
