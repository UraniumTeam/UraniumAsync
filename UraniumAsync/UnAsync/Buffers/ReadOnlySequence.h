#pragma once
#include <UnAsync/Buffers/IReadOnlySequenceSegment.h>

namespace UN::Async
{
    template<class T>
    class SequencePosition
    {
        IReadOnlySequenceSegment<T>* m_pSegment;
        USize m_Index;

    public:
        inline SequencePosition()
            : m_pSegment(nullptr)
            , m_Index(0)
        {
        }

        inline SequencePosition(IReadOnlySequenceSegment<T>* pSegment, USize index)
            : m_pSegment(pSegment)
            , m_Index(index)
        {
        }

        inline void AdvanceSegment()
        {
            m_pSegment = m_pSegment->Next();
        }

        inline IReadOnlySequenceSegment<T>* Segment() const noexcept
        {
            return m_pSegment;
        }

        [[nodiscard]] inline USize Index() const noexcept
        {
            return m_Index;
        }

        inline friend bool operator==(const SequencePosition<T>& lhs, const SequencePosition<T>& rhs) noexcept
        {
            return lhs.m_pSegment == rhs.m_pSegment && lhs.m_Index == rhs.m_Index;
        }

        inline friend bool operator!=(const SequencePosition<T>& lhs, const SequencePosition<T>& rhs) noexcept
        {
            return !(lhs == rhs);
        }

        inline friend USize operator-(const SequencePosition<T>& end, const SequencePosition<T>& start) noexcept
        {
            return (end.Segment()->RunningIndex() + end.Index()) - (start.Segment()->RunningIndex() + start.Index());
        }

        inline friend USize operator-(const SequencePosition<T>& end, USize startPosition) noexcept
        {
            return (end.Segment()->RunningIndex() + end.Index()) - startPosition;
        }
    };

    template<class T>
    class ReadOnlySequence final
    {
        SequencePosition<T> m_Begin;
        SequencePosition<T> m_End;

        [[nodiscard]] inline SequencePosition<T> SeekSingle(const SequencePosition<T>& pos, USize offset) const noexcept
        {
            return SequencePosition<T>(pos.Segment(), pos.Index() + offset);
        }

        [[nodiscard]] inline SequencePosition<T> SeekMultiple(IReadOnlySequenceSegment<T>* pCurrent,
                                                              const SequencePosition<T>& endPosition, USize offset) const
        {
            while (pCurrent != nullptr && pCurrent != endPosition.Segment())
            {
                auto length = pCurrent->Length();
                if (length > offset)
                {
                    return SequencePosition<T>(pCurrent, offset);
                }

                offset -= length;
                pCurrent = pCurrent->Next();
            }

            UN_Assert(pCurrent != nullptr && endPosition.Index() >= offset, "Out of range");
            return SequencePosition<T>(pCurrent, offset);
        }

        [[nodiscard]] inline bool TryGetBuffer(const SequencePosition<T>& position, ArraySlice<const T>& memory,
                                               SequencePosition<T>& next) const
        {
            if (position.Segment() == nullptr)
            {
                return false;
            }

            auto* pSegment = position.Segment();
            memory         = pSegment->GetMemory();

            if (pSegment != m_End.Segment())
            {
                next   = SequencePosition<T>(pSegment->Next(), 0);
                memory = memory(position.Index());
            }
            else
            {
                next   = SequencePosition<T>{};
                memory = memory(position.Index(), m_End.Index());
            }

            return true;
        }

    public:
        class Iterator final
        {
            friend class ReadOnlySequence<T>;

            const ReadOnlySequence<T>* m_pSequence;
            SequencePosition<T> m_Current;
            ArraySlice<const T> m_CurrentBuffer;

            inline Iterator(const ReadOnlySequence<T>* pSequence, const SequencePosition<T>& current)
                : m_pSequence(pSequence)
                , m_Current(current)
            {
                std::tie(m_CurrentBuffer, std::ignore) = pSequence->GetFirstBuffer();
            }

        public:
            inline operator SequencePosition<T>() // NOLINT(*-explicit-constructor)
            {
                return m_Current;
            }

            inline Iterator& operator++()
            {
                [[maybe_unused]] auto result = m_pSequence->TryGetNext(m_Current, m_CurrentBuffer);
                UN_Assert(result, "Couldn't move");
                return *this;
            }

            [[nodiscard]] inline ArraySlice<const T> operator*()
            {
                return m_CurrentBuffer;
            }

            [[nodiscard]] inline USize Index() const noexcept
            {
                return m_Current.Index();
            }

            [[nodiscard]] inline IReadOnlySequenceSegment<T>* Segment() const noexcept
            {
                return m_Current.Segment();
            }

            inline friend bool operator==(const Iterator& lhs, const Iterator& rhs) noexcept
            {
                return lhs.m_Current == rhs.m_Current;
            }

            inline friend bool operator!=(const Iterator& lhs, const Iterator& rhs) noexcept
            {
                return !(lhs == rhs);
            }
        };

        inline ReadOnlySequence() = default;

        inline ReadOnlySequence(const SequencePosition<T>& begin, const SequencePosition<T>& end)
            : m_Begin(begin)
            , m_End(end)
        {
        }

        [[nodiscard]] inline bool IsSingleSegment() const noexcept
        {
            return m_Begin.Segment() == m_End.Segment();
        }

        [[nodiscard]] inline ReadOnlySequence<T> operator()(const SequencePosition<T>& begin, const SequencePosition<T>& end)
        {
            return ReadOnlySequence<T>(begin, end);
        }

        [[nodiscard]] inline ReadOnlySequence<T> operator()(const SequencePosition<T>& begin)
        {
            return ReadOnlySequence<T>(begin, m_End);
        }

        [[nodiscard]] inline ReadOnlySequence<T> operator()(USize begin, USize end)
        {
            return ReadOnlySequence<T>(Seek(begin), Seek(end));
        }

        [[nodiscard]] inline ReadOnlySequence<T> operator()(USize begin)
        {
            return ReadOnlySequence<T>(Seek(begin), m_End);
        }

        [[nodiscard]] inline std::tuple<ArraySlice<const T>, SequencePosition<T>> GetFirstBuffer() const noexcept
        {
            if (m_Begin.Segment() == nullptr)
            {
                return std::make_tuple(ArraySlice<const T>{}, SequencePosition<T>{});
            }

            auto* pSegment = m_Begin.Segment();
            auto first     = pSegment->GetMemory();
            if (IsSingleSegment())
            {
                return std::make_tuple(first(m_Begin.Index(), m_End.Index()), SequencePosition<T>{});
            }

            return std::make_tuple(first(m_Begin.Index()), SequencePosition<T>(pSegment->Next(), 0));
        }

        [[nodiscard]] inline ArraySlice<const T> First() const
        {
            if (m_Begin.Segment() == nullptr)
            {
                return {};
            }

            auto* pSegment = m_Begin.Segment();
            auto first     = pSegment->GetMemory();
            if (IsSingleSegment())
            {
                return first(m_Begin.Index(), m_End.Index());
            }

            return first(m_Begin.Index());
        }

        [[nodiscard]] inline USize GetLength() const noexcept
        {
            if (m_Begin.Segment() == m_End.Segment())
            {
                return m_End.Index() - m_Begin.Index();
            }

            auto endOffset   = m_End.Segment()->RunningIndex() + m_End.Index();
            auto beginOffset = m_Begin.Segment()->RunningIndex() + m_Begin.Index();
            return endOffset - beginOffset;
        }

        [[nodiscard]] inline SequencePosition<T> Seek(const SequencePosition<T>& origin, USize offset) const
        {
            if (origin.Segment() == m_End.Segment())
            {
                return SeekSingle(origin, offset);
            }

            auto currentLength = origin.Segment()->Length() - origin.Index();
            if (currentLength > offset)
            {
                return SeekSingle(origin, offset);
            }

            return SeekMultiple(origin.Segment()->Next(), m_End, offset - currentLength);
        }

        inline bool TryGetNext(SequencePosition<T>& position, ArraySlice<const T>& memory, bool advance = true) const
        {
            SequencePosition<T> next;
            auto result = TryGetBuffer(position, memory, next);
            if (advance)
            {
                position = next;
            }

            return result;
        }

        inline void CopyDataTo(ArraySlice<T> destination)
        {
            UN_Assert(destination.Length() >= GetLength(), "Not enough memory");
            if (IsSingleSegment())
            {
                First().CopyDataTo(destination);
                return;
            }

            auto position = m_Begin;
            ArraySlice<const T> memory;
            while (TryGetNext(position, memory))
            {
                memory.CopyDataTo(destination);
                if (position.Segment() != nullptr)
                {
                    destination = destination(memory.Length());
                }
                else
                {
                    break;
                }
            }
        }

        [[nodiscard]] inline SequencePosition<T> BeginPosition() const noexcept
        {
            return m_Begin;
        }

        [[nodiscard]] inline SequencePosition<T> EndPosition() const noexcept
        {
            return m_End;
        }

        [[nodiscard]] inline Iterator begin() const noexcept
        {
            return Iterator(this, m_Begin);
        }

        [[nodiscard]] inline Iterator end() const noexcept
        {
            return Iterator(this, SequencePosition<T>{});
        }
    };
} // namespace UN::Async
