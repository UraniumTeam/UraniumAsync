#include <UnAsync/Internal/CancellationState.h>
#include <UnTL/Memory/SystemAllocator.h>

namespace UN::Async::Internal
{
    CancellationState::CancellationState()
        : m_State(CancellationSourceRefIncrement)
    {
    }

    CancellationState* CancellationState::Create()
    {
        auto* allocator = SystemAllocator::Get();
        return new (allocator->Allocate(sizeof(CancellationState), alignof(CancellationState))) CancellationState();
    }

    CancellationState::~CancellationState()
    {
        UN_Assert((m_State.load(std::memory_order_relaxed) & CancellationRefCountMask) == 0, "There must zero references");
    }

    void CancellationState::DestructThis()
    {
        auto* allocator = SystemAllocator::Get();
        this->~CancellationState();
        allocator->Deallocate(this);
    }

    void CancellationState::AddTokenRef() noexcept
    {
        m_State.fetch_add(CancellationTokenRefIncrement, std::memory_order_relaxed);
    }

    void CancellationState::ReleaseToken() noexcept
    {
        UInt64 oldState = m_State.fetch_sub(CancellationTokenRefIncrement, std::memory_order_acq_rel);
        if ((oldState & CancellationRefCountMask) == CancellationTokenRefIncrement)
        {
            DestructThis();
        }
    }

    void CancellationState::AddSourceRef() noexcept
    {
        m_State.fetch_add(CancellationSourceRefIncrement, std::memory_order_relaxed);
    }

    void CancellationState::ReleaseSource() noexcept
    {
        UInt64 oldState = m_State.fetch_sub(CancellationSourceRefIncrement, std::memory_order_acq_rel);
        if ((oldState & CancellationRefCountMask) == CancellationSourceRefIncrement)
        {
            DestructThis();
        }
    }

    bool CancellationState::IsCancellationRequested() const noexcept
    {
        return (m_State.load(std::memory_order_acquire) & CancellationRequestedFlag) != 0;
    }

    bool CancellationState::CanBeCancelled() const noexcept
    {
        return (m_State.load(std::memory_order_acquire) & CanBeCancelledMask) != 0;
    }

    void CancellationState::Cancel()
    {
        UInt64 oldState = m_State.fetch_or(CancellationRequestedFlag, std::memory_order_seq_cst);
        if ((oldState & CancellationRequestedFlag) != 0)
        {
            return;
        }
    }
} // namespace UN::Async::Internal
