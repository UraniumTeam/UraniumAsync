#include <UnAsync/Cancellation/CancellationSource.h>
#include <UnAsync/Cancellation/CancellationToken.h>
#include <UnAsync/Jobs/JobScheduler.h>
#include <UnAsync/SyncWait.h>
#include <UnAsync/Task.h>
#include <UnAsync/WhenAll.h>
#include <iostream>

using namespace UN;
using namespace UN::Async;

Ptr<IJobScheduler> pScheduler = AllocateObject<JobScheduler>(4);

std::thread::id TestTask(int n)
{
    std::vector<int> test;

    for (int i = 0; i < n; ++i)
    {
        test.push_back(i);
    }

    return std::this_thread::get_id();
}

Task<std::thread::id> Test()
{
    co_await Job::Run(pScheduler.Get());
    std::cout << "start\n" << std::flush;

    co_return co_await Job::Run(pScheduler.Get(), &TestTask, 10'000);
}

Task<> Test1(const CancellationToken& cancellationToken)
{
    co_await Job::Run(pScheduler.Get());

    while (!cancellationToken.IsCancellationRequested())
    {
        auto [a, b, c] = co_await WhenAll(Test(), Test(), Test());
        std::cout << a << std::endl;
        std::cout << b << std::endl;
        std::cout << c << std::endl;
    }
}

Task<> CancellingTask(CancellationSource& source)
{
    co_await Job::Run(pScheduler.Get());
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(5s);
    source.Cancel();
}

Task<> TestAwait()
{
    co_await Job::Run(pScheduler.Get());

    CancellationSource cancellationSource;
    auto cancellationToken = cancellationSource.GetToken();

    auto f = [cancellationToken] {
        return Test1(cancellationToken);
    };

    co_await WhenAllReady(f(), f(), CancellingTask(cancellationSource));
}

int main()
{
    // TODO: for some reason the CancellingTask sometimes blocks the scheduler
    auto task = TestAwait();
    SyncWait(task);
    return 0;
}
