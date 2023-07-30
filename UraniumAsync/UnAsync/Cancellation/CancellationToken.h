#pragma once
#include <UnTL/Base/Base.h>

namespace UN::Async
{
    class CancellationSource;

    namespace Internal
    {
        class CancellationState;
    }

    class CancellationToken
    {
        friend class CancellationSource;
        Internal::CancellationState* m_pState;

        CancellationToken() noexcept;
        explicit CancellationToken(Internal::CancellationState* pState) noexcept;

    public:
        CancellationToken(const CancellationToken& other) noexcept;
        CancellationToken(CancellationToken&& other) noexcept;
        ~CancellationToken();

        CancellationToken& operator=(const CancellationToken& other) noexcept;
        CancellationToken& operator=(CancellationToken&& other) noexcept;

        [[nodiscard]] bool IsCancellationRequested() const noexcept;
        [[nodiscard]] bool CanBeCancelled() const noexcept;
    };
} // namespace UN::Async
