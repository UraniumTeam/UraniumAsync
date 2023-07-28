#pragma once
#include <UnAsync/Internal/WhenAllCounter.h>
#include <UnAsync/Traits.h>

namespace UN::Async::Internal
{
    template<typename TTaskContainer>
    class WhenAllReadyAwaiter;

    template<typename TResult>
    class WhenAllTask;

    template<typename TResult>
    class WhenAllTaskPromise final
    {
        inline void RethrowIfException()
        {
            if (m_Exception)
            {
                std::rethrow_exception(m_Exception);
            }
        }

        WhenAllCounter* m_Counter;
        std::exception_ptr m_Exception;
        std::add_pointer_t<TResult> m_Result;

    public:
        using coroutine_handle_t = std::coroutine_handle<WhenAllTaskPromise<TResult>>;

        inline WhenAllTaskPromise() noexcept {}

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
            class Notifier
            {
            public:
                inline bool await_ready() const noexcept
                {
                    return false;
                }

                inline void await_suspend(coroutine_handle_t coroutine) const noexcept
                {
                    coroutine.promise().m_Counter->NotifyAwaitableCompleted();
                }

                inline void await_resume() const noexcept {}
            };

            return Notifier{};
        }

        inline void unhandled_exception() noexcept
        {
            m_Exception = std::current_exception();
        }

        inline void return_void() noexcept
        {
            UN_Assert(false, "");
        }

        inline auto yield_value(TResult&& result) noexcept
        {
            m_Result = std::addressof(result);
            return final_suspend();
        }

        inline void Start(WhenAllCounter& counter) noexcept
        {
            m_Counter = &counter;
            coroutine_handle_t::from_promise(*this).resume();
        }

        inline TResult& GetResult() &
        {
            RethrowIfException();
            return *m_Result;
        }

        inline TResult&& GetResult() &&
        {
            RethrowIfException();
            return std::forward<TResult>(*m_Result);
        }
    };

    template<>
    class WhenAllTaskPromise<void> final
    {
        WhenAllCounter* m_Counter;
        std::exception_ptr m_Exception;

    public:
        using coroutine_handle_t = std::coroutine_handle<WhenAllTaskPromise<void>>;

        inline WhenAllTaskPromise() noexcept {}

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
            class Notifier
            {
            public:
                inline bool await_ready() const noexcept
                {
                    return false;
                }

                inline void await_suspend(coroutine_handle_t coroutine) const noexcept
                {
                    coroutine.promise().m_Counter->NotifyAwaitableCompleted();
                }

                inline void await_resume() const noexcept {}
            };

            return Notifier{};
        }

        inline void unhandled_exception() noexcept
        {
            m_Exception = std::current_exception();
        }

        inline void return_void() noexcept {}

        inline void Start(WhenAllCounter& counter) noexcept
        {
            m_Counter = &counter;
            coroutine_handle_t::from_promise(*this).resume();
        }

        inline void GetResult()
        {
            if (m_Exception)
            {
                std::rethrow_exception(m_Exception);
            }
        }
    };

    template<typename TResult>
    class WhenAllTask final
    {
    public:
        using promise_type = WhenAllTaskPromise<TResult>;

        using coroutine_handle_t = typename promise_type::coroutine_handle_t;

    private:
        template<typename TTaskContainer>
        friend class WhenAllReadyAwaiter;

        inline void Start(WhenAllCounter& counter) noexcept
        {
            m_Coroutine.promise().Start(counter);
        }

        coroutine_handle_t m_Coroutine;

    public:
        inline WhenAllTask(coroutine_handle_t coroutine) noexcept
            : m_Coroutine(coroutine)
        {
        }

        inline WhenAllTask(WhenAllTask&& other) noexcept
            : m_Coroutine(std::exchange(other.m_Coroutine, coroutine_handle_t{}))
        {
        }

        inline ~WhenAllTask()
        {
            if (m_Coroutine)
                m_Coroutine.destroy();
        }

        inline WhenAllTask(const WhenAllTask&)            = delete;
        inline WhenAllTask& operator=(const WhenAllTask&) = delete;

        inline decltype(auto) GetResult() &
        {
            return m_Coroutine.promise().GetResult();
        }

        inline decltype(auto) GetResult() &&
        {
            return std::move(m_Coroutine.promise()).GetResult();
        }

        inline decltype(auto) GetNonVoidResult() &
        {
            if constexpr (std::is_void_v<decltype(this->GetResult())>)
            {
                this->GetResult();
                return EmptyStruct{};
            }
            else
            {
                return this->GetResult();
            }
        }

        inline decltype(auto) GetNonVoidResult() &&
        {
            if constexpr (std::is_void_v<decltype(this->GetResult())>)
            {
                std::move(*this).GetResult();
                return EmptyStruct{};
            }
            else
            {
                return std::move(*this).GetResult();
            }
        }
    };

    template<typename TAwaiter, typename TResult = typename AwaitableTraits<TAwaiter&&>::AwaitResultType,
             std::enable_if_t<!std::is_void_v<TResult>, int> = 0>
    inline WhenAllTask<TResult> MakeWhenAllTask(TAwaiter awaitable)
    {
        co_yield co_await static_cast<TAwaiter&&>(awaitable);
    }

    template<typename TAwaiter, typename TResult = typename AwaitableTraits<TAwaiter&&>::AwaitResultType,
             std::enable_if_t<std::is_void_v<TResult>, int> = 0>
    inline WhenAllTask<void> MakeWhenAllTask(TAwaiter awaitable)
    {
        co_await static_cast<TAwaiter&&>(awaitable);
    }

    template<typename TAwaiter, typename TResult = typename AwaitableTraits<TAwaiter&>::AwaitResultType,
             std::enable_if_t<!std::is_void_v<TResult>, int> = 0>
    inline WhenAllTask<TResult> MakeWhenAllTask(std::reference_wrapper<TAwaiter> awaitable)
    {
        co_yield co_await awaitable.get();
    }

    template<typename TAwaiter, typename TResult = typename AwaitableTraits<TAwaiter&>::AwaitResultType,
             std::enable_if_t<std::is_void_v<TResult>, int> = 0>
    inline WhenAllTask<void> MakeWhenAllTask(std::reference_wrapper<TAwaiter> awaitable)
    {
        co_await awaitable.get();
    }
} // namespace UN::Async::Internal
