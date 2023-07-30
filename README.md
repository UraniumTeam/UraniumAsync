# UraniumAsync
UraniumAsync is a C++20 parallel job system. The library supports work stealing, priority queue, job cancellation
trees, dependencies and coroutines.

### Examples
Use `Task<T>` and `co_await` to run code in parallel:
```cpp
// Create a scheduler
Ptr pScheduler = AllocateObject<JobScheduler>(4);

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
    // Schedule currently running coroutine
    co_await Job::Run(pScheduler.Get());
    std::cout << "start\n" << std::flush;

    // Run function as a task in thread pool
    co_return co_await Job::Run(pScheduler.Get(), &TestTask, 10'000'000);
}

Task<> Test1()
{
    co_await Job::Run(pScheduler.Get());

    // Run multiple jobs and join with co_await WhenAll
    auto [a, b, c] = co_await WhenAll(Test(), Test(), Test());
    std::cout << a << std::endl;
    std::cout << b << std::endl;
    std::cout << c << std::endl;
}

int main()
{
    auto task = Test1();
    SyncWait(task); // Wait for the jobs to finish
    return 0;
}
```
