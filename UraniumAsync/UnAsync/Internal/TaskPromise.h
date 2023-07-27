#pragma once
#include <UnAsync/Internal/TaskPromiseBase.h>
#include <concepts>
#include <exception>
#include <UnTL/Base/Base.h>

namespace UN::Async
{
    template<class T>
    class Task;
}

namespace UN::Async::Internal
{
    template<class T>
    class TaskPromise final : public TaskPromiseBase
    {
        enum class ResultType
        {
            Empty,
            Value,
            Exception
        };

        ResultType m_ResultType = ResultType::Empty;

        union
        {
            T m_Value;
            std::exception_ptr m_Exception;
        };

    public:
        inline TaskPromise() noexcept {}

        inline ~TaskPromise()
        {
            switch (m_ResultType)
            {
            case ResultType::Value:
                m_Value.~T();
                break;
            case ResultType::Exception:
                m_Exception.~exception_ptr();
                break;
            default:
                break;
            }
        }

        inline Task<T> get_return_object() noexcept;

        inline void unhandled_exception() noexcept
        {
            ::new (static_cast<void*>(std::addressof(m_Exception))) std::exception_ptr(std::current_exception());
            m_ResultType = ResultType::Exception;
        }

        template<class TValue>
        requires(std::convertible_to<TValue&&, T>) inline void return_value(TValue&& value) noexcept(
            std::is_nothrow_constructible_v<T, TValue&&>)
        {
            ::new (static_cast<void*>(std::addressof(m_Value))) T(std::forward<TValue>(value));
            m_ResultType = ResultType::Value;
        }

        inline T& GetResult() &
        {
            if (m_ResultType == ResultType::Exception)
            {
                std::rethrow_exception(m_Exception);
            }

            UN_Assert(m_ResultType == ResultType::Value, "Unexpected result type");
            return m_Value;
        }

        inline T&& GetResult() &&
        {
            if (m_ResultType == ResultType::Exception)
            {
                std::rethrow_exception(m_Exception);
            }

            UN_Assert(m_ResultType == ResultType::Value, "Unexpected result type");
            return std::move(m_Value);
        }
    };

    template<>
    class TaskPromise<void> : public TaskPromiseBase
    {
        std::exception_ptr m_Exception;

    public:
        inline TaskPromise() noexcept = default;

        inline Task<void> get_return_object() noexcept;

        inline void return_void() noexcept {}

        inline void unhandled_exception() noexcept
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
    };

    template<class T>
    class TaskPromise<T&> : public TaskPromiseBase
    {
        T* m_pValue = nullptr;
        std::exception_ptr m_Exception;

    public:
        inline TaskPromise() noexcept = default;

        inline Task<T&> get_return_object() noexcept;

        inline void unhandled_exception() noexcept
        {
            m_Exception = std::current_exception();
        }

        inline void return_value(T& value) noexcept
        {
            m_pValue = std::addressof(value);
        }

        inline T& GetResult()
        {
            if (m_Exception)
            {
                std::rethrow_exception(m_Exception);
            }

            return *m_pValue;
        }
    };
} // namespace UN::Async::Internal
