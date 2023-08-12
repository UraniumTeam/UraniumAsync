#pragma once
#include <UnAsync/Buffers/ReadOnlySequence.h>
#include <UnTL/Containers/HeapArray.h>
#include <optional>

namespace UN::Async
{
    template<class T>
    class SequenceReader final
    {
        ReadOnlySequence<T> m_Sequence;
        SequencePosition<T> m_CurrentPosition;
        SequencePosition<T> m_NextPosition;
        bool m_MoreData;
        mutable USize m_Length;
        USize m_Consumed;

        ArraySlice<const T> m_CurrentBuffer;
        USize m_CurrentBufferIndex;

        inline void MoveToNextBuffer()
        {
            if (!m_Sequence.IsSingleSegment())
            {
                auto previousNextPosition = m_NextPosition;
                ArraySlice<const T> memory;
                while (m_Sequence.TryGetNext(m_NextPosition, memory, true))
                {
                    m_CurrentPosition = previousNextPosition;
                    if (memory.Length() > 0)
                    {
                        m_CurrentBuffer      = memory;
                        m_CurrentBufferIndex = 0;
                        return;
                    }
                    else
                    {
                        m_CurrentBuffer      = ArraySlice<T>{};
                        m_CurrentBufferIndex = 0;
                        previousNextPosition = m_NextPosition;
                    }
                }
            }
            m_MoreData = false;
        }

        UN_FINLINE void AdvanceCurrentBuffer(USize count)
        {
            m_Consumed += count;
            m_CurrentBufferIndex += count;

            if (m_CurrentBufferIndex >= m_CurrentBuffer.Length())
            {
                MoveToNextBuffer();
            }
        }

        inline bool TryReadToImpl(ReadOnlySequence<T>& sequence, const T& delimiter, bool advancePastDelimiter, USize skip = 0)
        {
            auto copy = *this;
            if (skip > 0)
            {
                Advance(skip);
            }

            auto remaining = UnreadBuffer();
            while (m_MoreData)
            {
                auto index = remaining.FindFirstOf(delimiter);
                if (index != -1)
                {
                    if (index > 0)
                    {
                        AdvanceCurrentBuffer(index);
                    }

                    sequence = m_Sequence(copy.Position(), Position());
                    if (advancePastDelimiter)
                    {
                        Advance(1);
                    }

                    return true;
                }

                AdvanceCurrentBuffer(remaining.Length());
                remaining = m_CurrentBuffer;
            }

            *this = copy;
            return false;
        }

    public:
        inline explicit SequenceReader(const ReadOnlySequence<T>& sequence)
            : m_Sequence(sequence)
            , m_CurrentPosition(sequence.begin())
            , m_Length(std::numeric_limits<USize>::max())
            , m_Consumed(0)
            , m_CurrentBufferIndex(0)
        {
            std::tie(m_CurrentBuffer, m_NextPosition) = sequence.GetFirstBuffer();
            m_MoreData                                = m_CurrentBuffer.Length() > 0;

            if (!m_MoreData && !sequence.IsSingleSegment())
            {
                m_MoreData = true;
                MoveToNextBuffer();
            }
        }

        [[nodiscard]] inline bool IsEnd() const noexcept
        {
            return !m_MoreData;
        }

        [[nodiscard]] inline const ArraySlice<const T>& CurrentBuffer() const noexcept
        {
            return m_CurrentBuffer;
        }

        [[nodiscard]] inline ArraySlice<const T> UnreadBuffer() const noexcept
        {
            return m_CurrentBuffer(m_CurrentBufferIndex, m_CurrentBuffer.Length());
        }

        [[nodiscard]] inline USize CurrentBufferIndex() const noexcept
        {
            return m_CurrentBufferIndex;
        }

        [[nodiscard]] inline const ReadOnlySequence<T>& Sequence() const noexcept
        {
            return m_Sequence;
        }

        [[nodiscard]] inline ReadOnlySequence<T> UnreadSequence() const noexcept
        {
            return m_Sequence(Position());
        }

        [[nodiscard]] inline SequencePosition<T> Position() const noexcept
        {
            return m_Sequence.Seek(m_CurrentPosition, m_CurrentBufferIndex);
        }

        [[nodiscard]] inline USize Consumed() const noexcept
        {
            return m_Consumed;
        }

        [[nodiscard]] inline USize Length() const noexcept
        {
            [[unlikely]] if (m_Length == std::numeric_limits<USize>::max())
            {
                m_Length = m_Sequence.GetLength();
            }

            return m_Length;
        }

        inline void Advance(USize count) noexcept
        {
            if (m_CurrentBuffer.Length() - m_CurrentBufferIndex > count)
            {
                m_CurrentBufferIndex += count;
                m_Consumed += count;
                return;
            }

            while (m_MoreData)
            {
                auto remaining = m_CurrentBuffer.Length() - m_CurrentBufferIndex;

                if (remaining > count)
                {
                    m_CurrentBufferIndex += count;
                    count = 0;
                    break;
                }

                m_CurrentBufferIndex += remaining;
                count -= remaining;

                MoveToNextBuffer();

                if (count == 0)
                {
                    break;
                }
            }

            UN_Assert(count == 0, "Out of range");
        }

        [[nodiscard]] inline std::optional<T> Peek() const noexcept
        {
            if (m_MoreData)
            {
                return m_CurrentBuffer[m_CurrentBufferIndex];
            }

            return std::nullopt;
        }

        template<class TFunc>
        inline USize AdvancePastPredicate(TFunc&& predicate)
        {
            auto begin = m_Consumed;

            do
            {
                USize i;
                for (i = m_CurrentBufferIndex; i < m_CurrentBuffer.Length() && predicate(m_CurrentBuffer[i]); ++i)
                {
                }

                int advanced = i - m_CurrentBufferIndex;
                if (advanced == 0)
                {
                    break;
                }

                AdvanceCurrentBuffer(advanced);
            }
            while (m_CurrentBufferIndex == 0 && !IsEnd());

            return m_Consumed - begin;
        }

        inline USize AdvancePast(const T& value)
        {
            return AdvancePastPredicate([&value](const T& x) {
                return x == value;
            });
        }

        template<class... TValues>
        requires(std::same_as<TValues, T>&&...) inline USize AdvancePastAny(const TValues&... values)
        {
            return AdvancePastPredicate([&](const T& value) {
                return (... || (value == values));
            });
        }

        inline USize AdvancePastAny(const ArraySlice<const T>& values)
        {
            return AdvancePastPredicate([&values](const T& value) {
                return values.Contains(value);
            });
        }

        //! \brief Try to read the data up to a delimiter.
        //!
        //! This function may allocate a buffer to store the data up to the given delimiter if
        //! the next delimiter was found in one of the next buffers.
        //!
        //! \param [out] buffer - The read data.
        //! \param [out] owner - The read data owner (if the data was allocated).
        //! \param delimiter - The delimiter to read the data up to.
        //! \param advancePastDelimiter - If true, the reader's position will be moved past the delimiter.
        //!
        //! \return True if the delimiter was found.
        inline bool TryReadTo(ArraySlice<const T>& buffer, HeapArray<T>& owner, const T& delimiter,
                              bool advancePastDelimiter = true)
        {
            auto remaining = UnreadBuffer();
            auto index     = remaining.FindFirstOf(delimiter);

            if (index != -1)
            {
                buffer = index == 0 ? ArraySlice<const T>{} : remaining(0, index);
                AdvanceCurrentBuffer(index + (advancePastDelimiter ? 1 : 0));
                return true;
            }

            ReadOnlySequence<T> sequence;
            if (!TryReadToImpl(sequence, delimiter, advancePastDelimiter, m_CurrentBuffer.Length() - m_CurrentBufferIndex))
            {
                return false;
            }

            if (sequence.IsSingleSegment())
            {
                buffer = sequence.First();
                return true;
            }

            owner.Resize(sequence.GetLength());
            sequence.CopyDataTo(owner);
            buffer = owner;
            return true;
        }

        inline bool TryReadTo(ReadOnlySequence<T>& sequence, const T& delimiter, bool advancePastDelimiter = true)
        {
            return TryReadToImpl(sequence, delimiter, advancePastDelimiter);
        }
    };
} // namespace UN::Async
