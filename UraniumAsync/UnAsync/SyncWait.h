#pragma once
#include <UnAsync/Internal/SyncWaitTask.h>

namespace UN::Async
{
    template<typename TAwaitable>
    inline auto SyncWait(TAwaitable&& awaitable) -> typename AwaitableTraits<TAwaitable&&>::AwaitResultType
    {
        auto task = Internal::MakeSyncWaitTask(std::forward<TAwaitable>(awaitable));
        Internal::ManualResetEvent event;
        task.Start(event);
        event.Wait();
        return task.GetResult();
    }
} // namespace UN::Async
