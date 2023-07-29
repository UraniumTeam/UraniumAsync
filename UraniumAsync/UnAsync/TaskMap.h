#pragma once
#include <UnAsync/Internal/TaskMapAwaiter.h>

namespace UN::Async
{
    template<typename TFunc, Awaitable TAwaitable>
    inline auto MapTask(TFunc&& func, TAwaitable&& awaitable)
    {
        return Internal::TaskMapAwaitable<std::remove_cv_t<std::remove_reference_t<TFunc>>,
                                          std::remove_cv_t<std::remove_reference_t<TAwaitable>>>(
            std::forward<TFunc>(func), std::forward<TAwaitable>(awaitable));
    }
} // namespace UN::Async
