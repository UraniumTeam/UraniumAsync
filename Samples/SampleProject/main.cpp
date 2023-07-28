#include <UnAsync/SyncWait.h>
#include <UnAsync/Task.h>
#include <iostream>
#include <UnAsync/Jobs/JobScheduler.h>

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
    std::cout << "start" << std::endl;

    co_return co_await Job::Run(pScheduler.Get(), &TestTask, 10'000'000);
}

Task<> Test1()
{
    co_await Job::Run(pScheduler.Get());
    std::cout << co_await Test() << std::endl;
    std::cout << co_await Test() << std::endl;
}

Task<> TestAwait()
{
    co_await Job::Run(pScheduler.Get());
    auto t1 = Test1();
    std::cout << "1" << std::endl;
    auto t2 = Test1();
    std::cout << "2" << std::endl;
    co_await t1;
    co_await t2;
}

int main()
{
    auto task = TestAwait();
    SyncWait(task);
    return 0;
}
