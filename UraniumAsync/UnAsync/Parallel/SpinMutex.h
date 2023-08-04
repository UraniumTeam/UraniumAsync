#pragma once
#include <UnTL/Base/Base.h>
#include <atomic>
#include <thread>

namespace UN::Async
{
    namespace Internal
    {
        inline constexpr Int32 MaxSpinLockPauseCount = 32;

        struct SpinLockWait final
        {
            Int32 Count;

            inline SpinLockWait() noexcept
                : Count(1)
            {
            }

            UN_FINLINE void Reset() noexcept
            {
                Count = 1;
            }

            UN_FINLINE void Wait() noexcept
            {
                if (Count <= MaxSpinLockPauseCount)
                {
                    for (Int32 i = 0; i < Count; ++i)
                    {
                        _mm_pause();
                    }

                    Count <<= 1;
                }
                else
                {
                    std::this_thread::yield();
                }
            }
        };
    } // namespace Internal

    class SpinMutex final
    {
        std::atomic_bool m_Flag;

    public:
        SpinMutex(const SpinMutex&)            = delete;
        SpinMutex& operator=(const SpinMutex&) = delete;

        UN_FINLINE explicit SpinMutex(bool locked = false) // NOLINT
            : m_Flag(locked)
        {
        }

        UN_FINLINE bool try_lock() noexcept
        {
            return !m_Flag.exchange(true, std::memory_order_acquire);
        }

        UN_FINLINE void lock() noexcept
        {
            if (!try_lock())
            {
                Internal::SpinLockWait wait;

                while (!try_lock())
                {
                    wait.Wait();
                }
            }
        }

        UN_FINLINE void unlock() noexcept
        {
            m_Flag.store(false, std::memory_order_release);
        }
    };
} // namespace UN::Async
