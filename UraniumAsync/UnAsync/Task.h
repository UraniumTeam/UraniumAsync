#pragma once
#include <UnAsync/Internal/TaskPromise.h>
#include <UnTL/Base/Base.h>

namespace UN::Async
{
    template<class T = void>
    class [[nodiscard]] Task
    {
    public:
        using promise_type = Internal::TaskPromise<T>;

        using value_type = T;

    private:
        std::coroutine_handle<promise_type> m_Coroutine;

        struct AwaiterBase
        {
            std::coroutine_handle<promise_type> m_Coroutine;

            inline AwaiterBase(std::coroutine_handle<promise_type> coroutine) noexcept
                : m_Coroutine(coroutine)
            {
            }

            [[nodiscard]] inline bool await_ready() const noexcept
            {
                return !m_Coroutine || m_Coroutine.done();
            }

            inline bool await_suspend(std::coroutine_handle<> coroutine) noexcept
            {
                m_Coroutine.resume();
                return m_Coroutine.promise().TrySetContinuation(coroutine);
            }
        };

    public:
        inline Task() noexcept
            : m_Coroutine(nullptr)
        {
        }

        inline explicit Task(std::coroutine_handle<promise_type> coroutine)
            : m_Coroutine(coroutine)
        {
        }

        inline Task(Task&& t) noexcept
            : m_Coroutine(t.m_Coroutine)
        {
            t.m_Coroutine = nullptr;
        }

        inline Task(const Task&)            = delete;
        inline Task& operator=(const Task&) = delete;

        inline ~Task()
        {
            if (m_Coroutine)
            {
                m_Coroutine.destroy();
            }
        }

        inline Task& operator=(Task&& other) noexcept
        {
            if (std::addressof(other) != this)
            {
                if (m_Coroutine)
                {
                    m_Coroutine.destroy();
                }

                m_Coroutine       = other.m_Coroutine;
                other.m_Coroutine = nullptr;
            }

            return *this;
        }

        [[nodiscard]] inline bool IsReady() const noexcept
        {
            return !m_Coroutine || m_Coroutine.done();
        }

        inline auto operator co_await() const& noexcept
        {
            struct Awaiter : AwaiterBase
            {
                using AwaiterBase::AwaiterBase;

                inline decltype(auto) await_resume()
                {
                    return this->m_Coroutine.promise().GetResult();
                }
            };

            return Awaiter{ m_Coroutine };
        }

        inline auto operator co_await() const&& noexcept
        {
            struct Awaiter : AwaiterBase
            {
                using AwaiterBase::AwaiterBase;

                decltype(auto) await_resume()
                {
                    return std::move(this->m_Coroutine.promise()).GetResult();
                }
            };

            return Awaiter{ m_Coroutine };
        }

        inline auto WhenReady() const noexcept
        {
            struct Awaiter : AwaiterBase
            {
                using AwaiterBase::AwaiterBase;

                void await_resume() const noexcept {}
            };

            return Awaiter{ m_Coroutine };
        }
    };

    namespace Internal
    {
        template<class T>
        inline Task<T> TaskPromise<T>::get_return_object() noexcept
        {
            return Task<T>{ std::coroutine_handle<TaskPromise>::from_promise(*this) };
        }

        inline Task<void> TaskPromise<void>::get_return_object() noexcept
        {
            return Task<void>{ std::coroutine_handle<TaskPromise>::from_promise(*this) };
        }

        template<class T>
        inline Task<T&> TaskPromise<T&>::get_return_object() noexcept
        {
            return Task<T&>{ std::coroutine_handle<TaskPromise>::from_promise(*this) };
        }
    } // namespace Internal
} // namespace UN::Async
