#pragma once
#include <UnAsync/Pipes/Pipe.h>

namespace UN::Async
{
    class PipeReader final
    {
        Pipe* m_pPipe;

    public:
        inline explicit PipeReader(Pipe* pPipe)
            : m_pPipe(pPipe)
        {
        }

        inline Task<PipeReadResult> ReadAsync(const std::stop_token& token) const
        {
            return m_pPipe->ReadAsync(token);
        }

        inline void Advance(const SequencePosition<Byte>& consumed, const SequencePosition<Byte>& examined) const
        {
            m_pPipe->AdvanceReader(consumed, examined);
        }

        inline void Advance(const SequencePosition<Byte>& consumed) const
        {
            m_pPipe->AdvanceReader(consumed);
        }

        inline void Complete() const
        {
            m_pPipe->CompleteReader();
        }
    };
} // namespace UN::Async
