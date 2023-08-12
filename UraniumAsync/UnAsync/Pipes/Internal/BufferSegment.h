#pragma once
#include <UnAsync/Buffers/IReadOnlySequenceSegment.h>
#include <UnTL/Base/Byte.h>
#include <UnTL/Containers/ArraySlice.h>

namespace UN::Async::Internal
{
    class BufferSegment final : public IReadOnlySequenceSegment<Byte>
    {
        ArraySlice<Byte> m_Memory;
        BufferSegment* m_pNext = nullptr;
        USize m_End            = 0;
        USize m_RunningIndex   = 0;

    public:
        inline ~BufferSegment() final
        {
            Reset();
        }

        inline void Reset()
        {
            m_Memory       = {};
            m_pNext        = nullptr;
            m_End          = 0;
            m_RunningIndex = 0;
        }

        [[nodiscard]] inline USize Length() const override
        {
            return m_End;
        }

        [[nodiscard]] inline USize End() const
        {
            return m_End;
        }

        [[nodiscard]] inline USize RunningIndex() const override
        {
            return m_RunningIndex;
        }

        inline void AdvanceEnd(USize count)
        {
            auto end = m_End + count;
            UN_Assert(end <= m_Memory.Length(), "Out of range");
            m_End = end;
        }

        [[nodiscard]] inline USize Capacity() const
        {
            return m_Memory.Length();
        }

        [[nodiscard]] inline BufferSegment* Next() const override
        {
            return m_pNext;
        }

        inline void SetNext(BufferSegment* pNext)
        {
            m_pNext = pNext;

            auto* segment = this;
            while (segment->Next() != nullptr)
            {
                segment->Next()->m_RunningIndex = segment->m_RunningIndex + segment->End();

                segment = segment->Next();
            }
        }

        inline void SetMemory(const ArraySlice<Byte>& memory)
        {
            m_Memory = memory;
        }

        [[nodiscard]] inline ArraySlice<const Byte> GetMemory() const override
        {
            return m_Memory(0, m_End);
        }

        [[nodiscard]] inline ArraySlice<Byte> GetAvailableMemory() const
        {
            return m_Memory;
        }
    };
} // namespace UN::Async::Internal
