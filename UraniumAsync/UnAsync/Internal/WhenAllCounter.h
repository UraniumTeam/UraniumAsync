#pragma once
#include <UnTL/Base/Base.h>
#include <atomic>
#include <coroutine>

namespace UN::Async::Internal
{
    class WhenAllCounter
    {
    protected:
        std::atomic<USize> m_Count;
        std::coroutine_handle<> m_AwaitingCoroutine;

    public:
        inline explicit WhenAllCounter(USize count) noexcept
            : m_Count(count + 1)
            , m_AwaitingCoroutine(nullptr)
        {
        }

        inline bool IsReady() const noexcept
        {
            return static_cast<bool>(m_AwaitingCoroutine);
        }

        inline bool TryAwait(std::coroutine_handle<> awaitingCoroutine) noexcept
        {
            m_AwaitingCoroutine = awaitingCoroutine;
            return m_Count.fetch_sub(1, std::memory_order_acq_rel) > 1;
        }

        inline void NotifyAwaitableCompleted() noexcept
        {
            if (m_Count.fetch_sub(1, std::memory_order_acq_rel) == 1)
            {
                m_AwaitingCoroutine.resume();
            }
        }
    };
} // namespace UN::Async::Internal
