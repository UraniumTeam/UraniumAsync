#pragma once
#include <UnTL/Base/Base.h>
#include <atomic>
#include <cstdint>

namespace UN::Async::Internal
{
    class ManualResetEvent final
    {
        std::atomic<UInt8> m_Value;

    public:
        explicit ManualResetEvent(bool initial = false);
        ~ManualResetEvent() = default;

        void Set() noexcept;
        void Reset() noexcept;
        void Wait() noexcept;
    };
} // namespace UN::Async::Internal
