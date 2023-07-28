#pragma once
#include <UnAsync/Internal/WhenAllTask.h>
#include <UnAsync/Internal/WhenAllReadyAwaiter.h>
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
    template<class... TAwaiters>
    requires(Awaitable<Internal::UnwrapReference<std::remove_reference_t<TAwaiters>>>&&...)
    [[nodiscard]] UN_FINLINE auto WhenAllReady(TAwaiters&&... awaitables)
    // clang-format on
    {
        using Tuple = std::tuple<Internal::WhenAllTask<
            typename AwaitableTraits<Internal::UnwrapReference<std::remove_reference_t<TAwaiters>>>::AwaitResultType>...>;
        return Internal::WhenAllReadyAwaiter<Tuple>(
            std::make_tuple(Internal::MakeWhenAllTask(std::forward<TAwaiters>(awaitables))...));
    }
} // namespace UN::Async
