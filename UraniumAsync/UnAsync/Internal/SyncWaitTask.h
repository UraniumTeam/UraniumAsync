#pragma once
#include <UnAsync/Internal/ManualResetEvent.h>
#include <UnAsync/Traits.h>
#include <coroutine>

namespace UN::Async::Internal
{
    template<class TResult>
    class SyncWaitTask;

    template<class TResult>
    class SyncWaitTaskPromise final
    {
        using coroutine_handle_t = std::coroutine_handle<SyncWaitTaskPromise<TResult>>;

    public:
        using reference = TResult&&;

        inline SyncWaitTaskPromise() noexcept {}

        inline void Start(ManualResetEvent& event)
        {
            m_Event = &event;
            coroutine_handle_t::from_promise(*this).resume();
        }

        inline auto get_return_object() noexcept
        {
            return coroutine_handle_t::from_promise(*this);
        }

        inline std::suspend_always initial_suspend() noexcept
        {
            return {};
        }

        inline auto final_suspend() noexcept
        {
            struct Notifier
            {
                inline bool await_ready() const noexcept
                {
                    return false;
                }

                inline void await_suspend(coroutine_handle_t coroutine) const noexcept
                {
                    coroutine.promise().m_Event->Set();
                }

                inline void await_resume() noexcept {}
            };

            return Notifier{};
        }

        inline auto yield_value(reference result) noexcept
        {
            m_Result = std::addressof(result);
            return final_suspend();
        }

        inline void return_void() noexcept
        {
            // The coroutine should have either yielded a value or thrown
            // an exception in which case it should have bypassed return_void().
            assert(false);
        }

        inline void unhandled_exception()
        {
            m_Exception = std::current_exception();
        }

        inline reference GetResult()
        {
            if (m_Exception)
            {
                std::rethrow_exception(m_Exception);
            }

            return static_cast<reference>(*m_Result);
        }

    private:
        ManualResetEvent* m_Event;
        std::remove_reference_t<TResult>* m_Result;
        std::exception_ptr m_Exception;
    };

    template<>
    class SyncWaitTaskPromise<void>
    {
        using coroutine_handle_t = std::coroutine_handle<SyncWaitTaskPromise<void>>;

    public:
        inline SyncWaitTaskPromise() noexcept {}

        inline void Start(ManualResetEvent& event)
        {
            m_Event = &event;
            coroutine_handle_t::from_promise(*this).resume();
        }

        inline auto get_return_object() noexcept
        {
            return coroutine_handle_t::from_promise(*this);
        }

        inline std::suspend_always initial_suspend() noexcept
        {
            return {};
        }

        inline auto final_suspend() noexcept
        {
            class completion_notifier
            {
            public:
                bool await_ready() const noexcept
                {
                    return false;
                }

                void await_suspend(coroutine_handle_t coroutine) const noexcept
                {
                    coroutine.promise().m_Event->Set();
                }

                void await_resume() noexcept {}
            };

            return completion_notifier{};
        }

        inline void return_void() {}

        inline void unhandled_exception()
        {
            m_Exception = std::current_exception();
        }

        inline void GetResult()
        {
            if (m_Exception)
            {
                std::rethrow_exception(m_Exception);
            }
        }

    private:
        ManualResetEvent* m_Event;
        std::exception_ptr m_Exception;
    };

    template<class TResult>
    class SyncWaitTask final
    {
    public:
        using promise_type = SyncWaitTaskPromise<TResult>;

        using coroutine_handle_t = std::coroutine_handle<promise_type>;

        inline SyncWaitTask(coroutine_handle_t coroutine) noexcept
            : m_Coroutine(coroutine)
        {
        }

        inline SyncWaitTask(SyncWaitTask&& other) noexcept
            : m_Coroutine(std::exchange(other.m_Coroutine, coroutine_handle_t{}))
        {
        }

        inline ~SyncWaitTask()
        {
            if (m_Coroutine)
                m_Coroutine.destroy();
        }

        inline SyncWaitTask(const SyncWaitTask&)            = delete;
        inline SyncWaitTask& operator=(const SyncWaitTask&) = delete;

        inline void Start(ManualResetEvent& event) noexcept
        {
            m_Coroutine.promise().Start(event);
        }

        inline decltype(auto) GetResult()
        {
            return m_Coroutine.promise().GetResult();
        }

    private:
        coroutine_handle_t m_Coroutine;
    };

    template<class TAwaitable, class TResult = typename AwaitableTraits<TAwaitable&&>::AwaitResultType>
    requires(!std::is_void_v<TResult>) SyncWaitTask<TResult> MakeSyncWaitTask(TAwaitable&& awaitable)
    {
        co_yield co_await std::forward<TAwaitable>(awaitable);
    }

    template<class TAwaitable, class TResult = typename AwaitableTraits<TAwaitable&&>::AwaitResultType>
    requires(std::is_void_v<TResult>) SyncWaitTask<void> MakeSyncWaitTask(TAwaitable&& awaitable)
    {
        co_await std::forward<TAwaitable>(awaitable);
    }
} // namespace UN::Async::Internal
