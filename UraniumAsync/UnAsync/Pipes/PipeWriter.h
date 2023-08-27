#pragma once
#include <UnAsync/Pipes/Pipe.h>

namespace UN::Async
{
    class PipeWriter final
    {
        Pipe* m_pPipe;

    public:
        inline explicit PipeWriter(Pipe* pPipe)
            : m_pPipe(pPipe)
        {
        }

        [[nodiscard]] inline ArraySlice<Byte> GetMemory(USize sizeHint) const
        {
            return m_pPipe->GetMemory(sizeHint);
        }

        inline void Advance(USize byteCount) const
        {
            m_pPipe->Advance(byteCount);
        }

        inline Task<PipeFlushResult> FlushAsync(const std::stop_token& cancellationToken) const
        {
            return m_pPipe->FlushAsync(cancellationToken);
        }

        inline void Complete() const
        {
            m_pPipe->CompleteWriter();
        }
    };
} // namespace UN::Async
