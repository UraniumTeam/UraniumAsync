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

        inline IReadOnlySequenceSegment<T>* Segment() const
        {
            return m_pSegment;
        }

        [[nodiscard]] inline USize Index() const
        {
            return m_Index;
        }

        inline SequencePosition& operator++()
        {
            if (m_Index < m_pSegment->Length())
            {
                ++m_Index;
            }
            else
            {
                m_pSegment = m_pSegment->Next();
                m_Index    = 0;
            }

            return *this;
        }

        inline const T& operator*()
        {
            auto memory = m_pSegment->GetMemory();
            if (m_Index == memory.Length())
            {
                ++*this;
                memory = m_pSegment->GetMemory();
            }

            return memory[m_Index];
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
        inline ReadOnlySequence() = default;

        inline ReadOnlySequence(const SequencePosition<T>& begin, const SequencePosition<T>& end)
            : m_Begin(begin)
            , m_End(end)
        {
        }

        [[nodiscard]] inline SequencePosition<T> begin() const
        {
            return m_Begin;
        }

        [[nodiscard]] inline SequencePosition<T> end() const
        {
            return m_End;
        }
    };
} // namespace UN::Async
