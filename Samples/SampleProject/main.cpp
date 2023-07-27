#include <UnAsync/SyncWait.h>
#include <UnAsync/Task.h>
#include <iostream>

using namespace UN;
using namespace UN::Async;

Task<int> Test1()
{
    co_return 123;
}

Task<int> Test2(int a, int b)
{
    co_return a + b;
}

Task<> TestAwait()
{
    auto number = co_await Test1();
    std::cout << number << std::endl;
    std::cout << co_await Test2(number, number) << std::endl;
}

int main()
{
    auto task = TestAwait();
    SyncWait(task);
    return 0;
}
