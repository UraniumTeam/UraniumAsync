#pragma once
#include <UnAsync/Traits.h>

namespace UN::Async::Internal
{
    template<typename TFunc, typename TAwaitable>
    class TaskMapAwaiter
    {
        using AwaiterType = typename AwaitableTraits<TAwaitable&&>::AwaiterType;
        TFunc&& m_Func;
        AwaiterType m_awaiter;

    public:
        inline TaskMapAwaiter(TFunc&& func, TAwaitable&& awaitable) noexcept(
            std::is_nothrow_move_constructible_v<AwaiterType>&& noexcept(
                Internal::GetAwaiter(static_cast<TAwaitable&&>(awaitable))))
            : m_Func(static_cast<TFunc&&>(func))
            , m_awaiter(Internal::GetAwaiter(static_cast<TAwaitable&&>(awaitable)))
        {
        }

        inline decltype(auto) await_ready() noexcept(noexcept(static_cast<AwaiterType&&>(m_awaiter).await_ready()))
        {
            return static_cast<AwaiterType&&>(m_awaiter).await_ready();
        }

        template<typename TPromise>
        inline decltype(auto) await_suspend(std::coroutine_handle<TPromise> coroutine) noexcept(
            noexcept(static_cast<AwaiterType&&>(m_awaiter).await_suspend(std::move(coroutine))))
        {
            return static_cast<AwaiterType&&>(m_awaiter).await_suspend(std::move(coroutine));
        }

        template<typename TAwaitResult                               = decltype(std::declval<AwaiterType>().await_resume()),
                 std::enable_if_t<std::is_void_v<TAwaitResult>, int> = 0>
        inline decltype(auto) await_resume() noexcept(noexcept(std::invoke(static_cast<TFunc&&>(m_Func))))
        {
            static_cast<AwaiterType&&>(m_awaiter).await_resume();
            return std::invoke(static_cast<TFunc&&>(m_Func));
        }

        template<typename TAwaitResult                                = decltype(std::declval<AwaiterType>().await_resume()),
                 std::enable_if_t<!std::is_void_v<TAwaitResult>, int> = 0>
        inline decltype(auto) await_resume() noexcept(noexcept(std::invoke(static_cast<TFunc&&>(m_Func),
                                                                           static_cast<AwaiterType&&>(m_awaiter).await_resume())))
        {
            return std::invoke(static_cast<TFunc&&>(m_Func), static_cast<AwaiterType&&>(m_awaiter).await_resume());
        }
    };

    template<typename TFunc, typename TAwaitable>
    class TaskMapAwaitable
    {
        static_assert(!std::is_lvalue_reference_v<TFunc>);
        static_assert(!std::is_lvalue_reference_v<TAwaitable>);

        TFunc m_Func;
        TAwaitable m_Awaitable;

    public:
        template<typename TFuncArg, typename TAwaitableArg,
                 std::enable_if_t<
                     std::is_constructible_v<TFunc, TFuncArg&&> && std::is_constructible_v<TAwaitable, TAwaitableArg&&>, int> = 0>
        inline explicit TaskMapAwaitable(TFuncArg&& func, TAwaitableArg&& awaitable) noexcept(
            std::is_nothrow_constructible_v<TFunc, TFuncArg&&> && std::is_nothrow_constructible_v<TAwaitable, TAwaitableArg&&>)
            : m_Func(static_cast<TFuncArg&&>(func))
            , m_Awaitable(static_cast<TAwaitableArg&&>(awaitable))
        {
        }

        inline auto operator co_await() const&
        {
            return TaskMapAwaiter<const TFunc&, const TAwaitable&>(m_Func, m_Awaitable);
        }

        inline auto operator co_await() &
        {
            return TaskMapAwaiter<TFunc&, TAwaitable&>(m_Func, m_Awaitable);
        }

        inline auto operator co_await() &&
        {
            return TaskMapAwaiter<TFunc&&, TAwaitable&&>(static_cast<TFunc&&>(m_Func), static_cast<TAwaitable&&>(m_Awaitable));
        }
    };
} // namespace UN::Async::Internal
