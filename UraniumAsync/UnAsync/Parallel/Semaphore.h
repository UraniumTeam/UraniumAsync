#pragma once
#include <UnTL/Base/Base.h>

#if UN_LINUX
#    include <semaphore.h>
#endif

namespace UN::Async
{
    class Semaphore
    {
#if UN_WINDOWS
        void* m_pNativeSemaphore;
#else
        sem_t m_NativeSemaphore;
#endif
    public:
        explicit Semaphore(UInt32 initialValue = 0);
        ~Semaphore();

        void Acquire();
        void Release(UInt32 count = 1);
    };
} // namespace UN::Async
