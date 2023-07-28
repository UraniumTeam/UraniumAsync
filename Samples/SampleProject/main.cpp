#include <UnAsync/SyncWait.h>
#include <UnAsync/Task.h>
#include <iostream>
#include <UnAsync/Jobs/JobScheduler.h>

using namespace UN;
using namespace UN::Async;

Ptr<IJobScheduler> pScheduler = AllocateObject<JobScheduler>(4);

Task<std::thread::id> Test()
{
    co_await Job::Run(pScheduler.Get());

    std::vector<int> test;
    for (int i = 0; i < 10'000'000; ++i)
    {
        test.push_back(i);
    }

    co_return std::this_thread::get_id();
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
    co_await Test1();
    std::cout << "1" << std::endl;
    co_await Test1();
    std::cout << "2" << std::endl;
}

int main()
{
    auto task = TestAwait();
    SyncWait(task);
    return 0;
}
