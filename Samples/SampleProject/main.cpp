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

    co_return co_await Job::Run(pScheduler.Get(), &TestTask, 10'000'000);
}

Task<> Test1()
{
    co_await Job::Run(pScheduler.Get());

    auto [a, b, c] = co_await WhenAllReady(Test(), Test(), Test());
    std::cout << a.GetResult() << std::endl;
    std::cout << b.GetResult() << std::endl;
    std::cout << c.GetResult() << std::endl;
}

Task<> TestAwait()
{
    co_await Job::Run(pScheduler.Get());
    co_await WhenAllReady(Test1(), Test1());
}

int main()
{
    auto task = TestAwait();
    SyncWait(task);
    return 0;
}
