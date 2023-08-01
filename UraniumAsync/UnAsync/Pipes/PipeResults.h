#pragma once
#include <UnTL/Base/Base.h>
#include <UnTL/Base/Byte.h>
#include <UnTL/Base/Flags.h>
#include <UnTL/Containers/ArraySlice.h>

namespace UN::Async
{
    enum class PipeResultFlags : UInt8
    {
        None      = 0,
        Cancelled = UN_BIT(0),
        Completed = UN_BIT(1)
    };

    UN_ENUM_OPERATORS(PipeResultFlags);

    class PipeBaseResult
    {
        PipeResultFlags m_Flags;

    protected:
        inline explicit PipeBaseResult(PipeResultFlags flags)
            : m_Flags(flags)
        {
        }

    public:
        [[nodiscard]] inline bool IsCompleted() const
        {
            return AllFlagsActive(m_Flags, PipeResultFlags::Completed);
        }

        [[nodiscard]] inline bool IsCancelled() const
        {
            return AllFlagsActive(m_Flags, PipeResultFlags::Cancelled);
        }
    };

    class PipeFlushResult final : public PipeBaseResult
    {
    public:
        inline explicit PipeFlushResult(PipeResultFlags flags)
            : PipeBaseResult(flags)
        {
        }
    };

    class PipeReadResult final : public PipeBaseResult
    {
        ReadOnlySequence<Byte> m_Memory;

    public:
        inline explicit PipeReadResult(PipeResultFlags flags, const ReadOnlySequence<Byte>& memory)
            : PipeBaseResult(flags)
            , m_Memory(memory)
        {
        }

        [[nodiscard]] inline ReadOnlySequence<Byte> GetMemory() const
        {
            return m_Memory;
        }
    };
} // namespace UN::Async
