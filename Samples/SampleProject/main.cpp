#include <UnAsync/Jobs/JobScheduler.h>
#include <UnAsync/SyncWait.h>
#include <UnAsync/Task.h>
#include <UnAsync/WhenAll.h>
#include <iostream>
#include <stop_token>

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

Task<> Test1(const std::stop_token& cancellationToken)
{
    co_await Job::Run(pScheduler.Get());

    while (!cancellationToken.stop_requested())
    {
        auto [a, b, c] = co_await WhenAll(Test(), Test(), Test());
        std::cout << a << std::endl;
        std::cout << b << std::endl;
        std::cout << c << std::endl;
    }
}

void CancellingTask(std::stop_source& source)
{
    std::cout << "Cancelling task\n" << std::flush;
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(5s);
    source.request_stop();
}

Task<> TestAwait()
{
    co_await Job::Run(pScheduler.Get());

    std::stop_source cancellationSource;
    auto cancellationToken = cancellationSource.get_token();

    Job::RunOneTime(pScheduler.Get(), [&cancellationSource]() {
        CancellingTask(cancellationSource);
    });

    auto f = [cancellationToken] {
        return Test1(cancellationToken);
    };

    co_await WhenAllReady(f(), f());
}

int main()
{
    auto task = TestAwait();
    SyncWait(task);
    return 0;
}
