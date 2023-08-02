#include <UnAsync/AsyncEvent.h>
#include <UnTL/Base/Base.h>

namespace UN::Async
{
    AsyncEvent::AsyncEvent(bool initial) noexcept
        : m_State(initial ? static_cast<void*>(this) : nullptr)
    {
    }

    AsyncEvent::~AsyncEvent()
    {
        UN_Assert(m_State.load(std::memory_order_relaxed) == nullptr
                      || m_State.load(std::memory_order_relaxed) == static_cast<void*>(this),
                  "The event is still being awaited");
    }

    AsyncEventOperation AsyncEvent::operator co_await() const noexcept
    {
        return AsyncEventOperation{ *this };
    }

    bool AsyncEvent::IsSet() const noexcept
    {
        return m_State.load(std::memory_order_acquire) == static_cast<const void*>(this);
    }

    void AsyncEvent::Set() noexcept
    {
        void* const setState = static_cast<void*>(this);
        void* oldState       = m_State.exchange(setState, std::memory_order_acq_rel);
        if (oldState != setState)
        {
            auto* current = static_cast<AsyncEventOperation*>(oldState);
            while (current != nullptr)
            {
                auto* next = current->m_Next;
                current->m_Awaiter.resume();
                current = next;
            }
        }
    }

    void AsyncEvent::Reset() noexcept
    {
        void* oldState = static_cast<void*>(this);
        m_State.compare_exchange_strong(oldState, nullptr, std::memory_order_relaxed);
    }

    AsyncEventOperation::AsyncEventOperation(const AsyncEvent& event) noexcept
        : m_Event(event)
    {
    }

    bool AsyncEventOperation::await_ready() const noexcept
    {
        return m_Event.IsSet();
    }

    bool AsyncEventOperation::await_suspend(std::coroutine_handle<> awaiter) noexcept
    {
        m_Awaiter = awaiter;

        const void* const setState = static_cast<const void*>(&m_Event);

        void* oldState = m_Event.m_State.load(std::memory_order_acquire);
        do
        {
            if (oldState == setState)
            {
                return false;
            }

            m_Next = static_cast<AsyncEventOperation*>(oldState);
        }
        while (!m_Event.m_State.compare_exchange_weak(
            oldState, static_cast<void*>(this), std::memory_order_release, std::memory_order_acquire));

        return true;
    }
} // namespace UN::Async
