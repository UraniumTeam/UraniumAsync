#pragma once

namespace UN::Async
{
    class CancellationToken;

    namespace Internal
    {
        class CancellationState;
    }

    class CancellationSource
    {
        Internal::CancellationState* m_pState;

    public:
        CancellationSource();
        CancellationSource(const CancellationSource& other) noexcept;
        CancellationSource(CancellationSource&& other) noexcept;
        ~CancellationSource();

        CancellationSource& operator=(const CancellationSource& other) noexcept;
        CancellationSource& operator=(CancellationSource&& other) noexcept;

        [[nodiscard]] bool IsCancellationRequested() const noexcept;
        [[nodiscard]] bool CanBeCancelled() const noexcept;
        [[nodiscard]] CancellationToken GetToken() const noexcept;

        void Cancel();
    };
} // namespace UN::Async
