#pragma once
#include <atomic>
#include <coroutine>

namespace UN::Async::Internal
{
    class TaskPromiseBase
    {
        std::coroutine_handle<> m_Continuation;
        std::atomic_bool m_State;

        struct FinalAwaiter
        {
            [[nodiscard]] inline bool await_ready() const noexcept
            {
                return false;
            }

            template<typename TPromise>
            inline void await_suspend(std::coroutine_handle<TPromise> coroutine) noexcept
            {
                TaskPromiseBase& promise = coroutine.promise();
                if (promise.m_State.exchange(true, std::memory_order_acq_rel))
                {
                    promise.m_Continuation.resume();
                }
            }

            inline void await_resume() noexcept {}
        };

    public:
        inline TaskPromiseBase() noexcept
            : m_State(false)
        {
        }

        inline auto initial_suspend() noexcept -> std::suspend_always
        {
            return {};
        }

        inline auto final_suspend() noexcept -> FinalAwaiter
        {
            return {};
        }

        inline bool TrySetContinuation(std::coroutine_handle<> continuation)
        {
            m_Continuation = continuation;
            return !m_State.exchange(true, std::memory_order_acq_rel);
        }
    };
} // namespace UN::Async::Internal
