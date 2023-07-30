#pragma once
#include <UnAsync/Cancellation/CancellationToken.h>
#include <atomic>

namespace UN::Async::Internal
{
    class CancellationState
    {
        inline static constexpr UInt64 CancellationRequestedFlag            = 1;
        inline static constexpr UInt64 CancellationNotificationCompleteFlag = 2;
        inline static constexpr UInt64 CancellationSourceRefIncrement       = 4;
        inline static constexpr UInt64 CancellationTokenRefIncrement        = static_cast<UInt64>(1) << 33;
        inline static constexpr UInt64 CanBeCancelledMask                   = CancellationTokenRefIncrement - 1;
        inline static constexpr UInt64 CancellationRefCountMask =
            ~(CancellationRequestedFlag | CancellationNotificationCompleteFlag);

        std::atomic<UInt64> m_State;

        CancellationState();
        ~CancellationState();

        void DestructThis();

    public:
        static CancellationState* Create();

        void AddTokenRef() noexcept;
        void ReleaseToken() noexcept;

        void AddSourceRef() noexcept;
        void ReleaseSource() noexcept;

        [[nodiscard]] bool IsCancellationRequested() const noexcept;
        [[nodiscard]] bool CanBeCancelled() const noexcept;

        void Cancel();
    };
} // namespace UN::Async::Internal
