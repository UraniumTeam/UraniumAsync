#pragma once
#include <any>
#include <concepts>
#include <coroutine>
#include <type_traits>

namespace UN::Async
{
    namespace Internal
    {
        template<class T>
        struct IsCoroutineHandle : std::false_type
        {
        };

        template<class TPromise>
        struct IsCoroutineHandle<std::coroutine_handle<TPromise>> : std::true_type
        {
        };

        template<class T>
        concept ValidAwaitSuspendReturnValue = std::same_as<T, bool> || IsCoroutineHandle<T>::value;

        // clang-format off
        template<class T>
        concept Awaiter = requires
        {
            { std::declval<T>().await_suspend(std::declval<std::coroutine_handle<>>()) } -> ValidAwaitSuspendReturnValue;
            { std::declval<T>().await_ready() } -> std::convertible_to<bool>;
            std::declval<T>().await_resume();
        };
        // clang-format on

        template<class T>
        concept HasAwaitMember = requires
        {
            static_cast<T&&>(std::declval<T>()).operator co_await();
        };

        template<class T>
        concept HasAwaitOperator = requires
        {
            operator co_await(static_cast<T&&>(std::declval<T>()));
        };

        template<HasAwaitMember T>
        inline auto GetAwaiterImpl(T&& value) noexcept(noexcept(static_cast<T&&>(value).operator co_await()))
        {
            return static_cast<T&&>(value).operator co_await();
        }

        template<HasAwaitOperator T>
        inline auto GetAwaiterImpl(T&& value) noexcept(noexcept(operator co_await(static_cast<T&&>(value))))
        {
            return operator co_await(static_cast<T&&>(value));
        }

        template<class T>
        requires(Awaiter<T&&>) inline T&& GetAwaiterImpl(T&& value) noexcept
        {
            return static_cast<T&&>(value);
        }

        template<class T>
        auto GetAwaiter(T&& value) noexcept(noexcept(GetAwaiterImpl(static_cast<T&&>(value))))
            -> decltype(GetAwaiterImpl(static_cast<T&&>(value)))
        {
            return GetAwaiterImpl(static_cast<T&&>(value));
        }
    } // namespace Internal

    template<class T>
    concept AwaitableTraitsWork = requires
    {
        Internal::GetAwaiter(std::declval<T>());
    };

    template<AwaitableTraitsWork T>
    struct AwaitableTraits
    {
        using AwaiterType     = decltype(Internal::GetAwaiter(std::declval<T>()));
        using AwaitResultType = decltype(std::declval<AwaiterType>().await_resume());
    };
} // namespace UN::Async
