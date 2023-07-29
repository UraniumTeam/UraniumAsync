#pragma once
#include <UnAsync/Internal/WhenAllReadyAwaiter.h>
#include <UnAsync/Internal/WhenAllTask.h>
#include <UnAsync/TaskMap.h>
#include <UnAsync/Traits.h>

namespace UN::Async
{
    namespace Internal
    {
        template<typename T>
        struct UnwrapReferenceImpl
        {
            using type = T;
        };

        template<typename T>
        struct UnwrapReferenceImpl<std::reference_wrapper<T>>
        {
            using type = T;
        };

        template<typename T>
        using UnwrapReference = typename UnwrapReferenceImpl<T>::type;
    } // namespace Internal

    // clang-format off
    template<class... TAwaitables>
    requires(Awaitable<Internal::UnwrapReference<std::remove_reference_t<TAwaitables>>>&&...)
    [[nodiscard]] UN_FINLINE auto WhenAllReady(TAwaitables&&... awaitables)
    // clang-format on
    {
        using Tuple = std::tuple<Internal::WhenAllTask<
            typename AwaitableTraits<Internal::UnwrapReference<std::remove_reference_t<TAwaitables>>>::AwaitResultType>...>;
        return Internal::WhenAllReadyAwaiter<Tuple>(
            std::make_tuple(Internal::MakeWhenAllTask(std::forward<TAwaitables>(awaitables))...));
    }

    template<class... TAwaitables>
    requires(Awaitable<Internal::UnwrapReference<std::remove_reference_t<TAwaitables>>>&&...)
        [[nodiscard]] auto WhenAll(TAwaitables&&... awaitables)
    {
        return MapTask(
            [](auto&& taskTuple) {
                return std::apply(
                    [](auto&&... tasks) {
                        return std::make_tuple(static_cast<decltype(tasks)>(tasks).GetNonVoidResult()...);
                    },
                    static_cast<decltype(taskTuple)>(taskTuple));
            },
            WhenAllReady(std::forward<TAwaitables>(awaitables)...));
    }
} // namespace UN::Async
