#pragma once
#include <atomic>
#include <coroutine>

namespace UN::Async
{
    class AsyncEventOperation;

    class AsyncEvent
    {
        friend class AsyncEventOperation;
        mutable std::atomic<void*> m_State;

    public:
        explicit AsyncEvent(bool initial = false) noexcept;
        ~AsyncEvent();

        AsyncEventOperation operator co_await() const noexcept;

        [[nodiscard]] bool IsSet() const noexcept;
        void Set() noexcept;
        void Reset() noexcept;
    };

    class AsyncEventOperation
    {
        friend class AsyncEvent;

        const AsyncEvent& m_Event;
        AsyncEventOperation* m_Next;
        std::coroutine_handle<> m_Awaiter;

    public:
        explicit AsyncEventOperation(const AsyncEvent& event) noexcept;

        bool await_ready() const noexcept;
        bool await_suspend(std::coroutine_handle<> awaiter) noexcept;
        void await_resume() const noexcept {}
    };
} // namespace UN::Async
