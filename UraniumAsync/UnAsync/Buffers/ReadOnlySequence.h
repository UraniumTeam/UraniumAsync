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

    public:
        class Iterator final
        {
            friend class ReadOnlySequence<T>;

            const ReadOnlySequence<T>* m_pSequence;
            SequencePosition<T> m_Current;

            inline Iterator(const ReadOnlySequence<T>* pSequence, const SequencePosition<T>& current)
                : m_pSequence(pSequence)
                , m_Current(current)
            {
            }

        public:
            inline operator SequencePosition<T>()
            {
                return m_Current;
            }

            inline Iterator& operator++()
            {
                if (m_Current.Segment() == m_pSequence->m_End.Segment())
                {
                    m_Current = m_pSequence->m_End;
                }
                else
                {
                    m_Current.AdvanceSegment();
                }
                return *this;
            }

            inline const ArraySlice<const T> operator*()
            {
                return m_Current.Segment()->GetMemory();
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

        [[nodiscard]] inline Iterator begin() const
        {
            return Iterator(this, m_Begin);
        }

        [[nodiscard]] inline Iterator end() const
        {
            return Iterator(this, m_End);
        }
    };
} // namespace UN::Async
