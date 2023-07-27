#include <UnAsync/Internal/ManualResetEvent.h>
#include <UnAsync/Internal/PlatformInclude.h>

#if UN_LINUX
#    include <cassert>
#    include <cerrno>
#    include <climits>

namespace
{
    int futex(int* UserAddress, int FutexOperation, int Value, const struct timespec* timeout, int* UserAddress2, int Value3)
    {
        return syscall(SYS_futex, UserAddress, FutexOperation, Value, timeout, UserAddress2, Value3);
    }
} // namespace
#endif

namespace UN::Async::Internal
{
    ManualResetEvent::ManualResetEvent(bool initial)
        : m_Value(initial ? 1 : 0)
    {
    }

    void ManualResetEvent::Set() noexcept
    {
#if UN_WINDOWS
        m_Value.store(1, std::memory_order_release);
        ::WakeByAddressAll(&m_Value);
#else
        m_Value.store(1, std::memory_order_release);

        constexpr int numberOfWaitersToWakeUp = INT_MAX;

        [[maybe_unused]] int numberOfWaitersWokenUp =
            futex(reinterpret_cast<int*>(&m_Value), FUTEX_WAKE_PRIVATE, numberOfWaitersToWakeUp, nullptr, nullptr, 0);

        UN_Assert(numberOfWaitersWokenUp != -1, "fail");
#endif
    }

    void ManualResetEvent::Reset() noexcept
    {
        m_Value.store(0, std::memory_order_relaxed);
    }

    void ManualResetEvent::Wait() noexcept
    {
#if UN_WINDOWS
        int value = m_Value.load(std::memory_order_acquire);
        BOOL ok   = TRUE;
        while (value == 0)
        {
            if (!ok)
            {
                ::Sleep(1);
            }

            ok    = ::WaitOnAddress(&m_Value, &value, sizeof(m_Value), INFINITE);
            value = m_Value.load(std::memory_order_acquire);
        }
#else
        int oldValue = m_Value.load(std::memory_order_acquire);
        while (oldValue == 0)
        {
            int result = futex(reinterpret_cast<int*>(&m_Value), FUTEX_WAIT_PRIVATE, oldValue, nullptr, nullptr, 0);
            if (result == -1)
            {
                if (errno == EAGAIN)
                {
                    return;
                }
            }

            oldValue = m_Value.load(std::memory_order_acquire);
        }
#endif
    }
} // namespace UN::Async::Internal
