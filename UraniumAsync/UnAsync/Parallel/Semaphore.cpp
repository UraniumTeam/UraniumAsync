#include <UnAsync/Internal/PlatformInclude.h>
#include <UnAsync/Parallel/Semaphore.h>

namespace UN::Async
{
#if UN_WINDOWS
    Semaphore::Semaphore(UInt32 initialValue)
    {
        m_pNativeSemaphore = CreateSemaphore(nullptr, static_cast<LONG>(initialValue), 0xfff, nullptr);
    }

    Semaphore::~Semaphore()
    {
        CloseHandle(m_pNativeSemaphore);
    }

    void Semaphore::Acquire()
    {
        WaitForSingleObject(m_pNativeSemaphore, static_cast<DWORD>(-1));
    }

    void Semaphore::Release(UInt32 count)
    {
        ReleaseSemaphore(m_pNativeSemaphore, static_cast<LONG>(count), nullptr);
    }
#else
    Semaphore::Semaphore(UInt32 initialValue)
    {
        [[maybe_unused]] int result = sem_init(&m_NativeSemaphore, 0, initialValue);
        UN_Assert(result == 0, "sem_init failed");
    }

    Semaphore::~Semaphore()
    {
        [[maybe_unused]] int result = sem_destroy(&m_NativeSemaphore);
        UN_Assert(result == 0, "sem_destroy failed");
    }

    void Semaphore::Acquire()
    {
        [[maybe_unused]] int result = sem_wait(&m_NativeSemaphore);
        UN_Assert(result == 0, "sem_wait failed");
    }

    void Semaphore::Release(UInt32 count)
    {
        while (count > 0)
        {
            int result = sem_post(&m_NativeSemaphore);
            UN_Assert(result == 0, "sem_post failed");

            if (result)
            {
                break;
            }

            --count;
        }
    }
#endif
} // namespace UN::Async
