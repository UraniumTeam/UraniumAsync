#include <UnAsync/Internal/PlatformInclude.h>
#include <UnAsync/Parallel/Semaphore.h>

namespace UN::Async
{
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
} // namespace UN::Async
