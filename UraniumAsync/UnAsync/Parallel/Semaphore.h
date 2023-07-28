#pragma once
#include <UnTL/Base/Base.h>

namespace UN::Async
{
    class Semaphore
    {
        void* m_pNativeSemaphore;

    public:
        explicit Semaphore(UInt32 initialValue = 0);
        ~Semaphore();

        void Acquire();
        void Release(UInt32 count = 1);
    };
} // namespace UN::Async
