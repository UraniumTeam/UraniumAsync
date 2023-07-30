#include <UnAsync/Cancellation/CancellationSource.h>
#include <UnAsync/Internal/CancellationState.h>

namespace UN::Async
{
    CancellationSource::CancellationSource()
        : m_pState(Internal::CancellationState::Create())
    {
    }

    CancellationSource::CancellationSource(const CancellationSource& other) noexcept
        : m_pState(other.m_pState)
    {
        if (m_pState)
        {
            m_pState->AddSourceRef();
        }
    }

    CancellationSource::CancellationSource(CancellationSource&& other) noexcept
        : m_pState(other.m_pState)
    {
        other.m_pState = nullptr;
    }

    CancellationSource::~CancellationSource()
    {
        if (m_pState)
        {
            m_pState->ReleaseSource();
        }
    }

    CancellationSource& CancellationSource::operator=(const CancellationSource& other) noexcept
    {
        if (m_pState != other.m_pState)
        {
            if (m_pState != nullptr)
            {
                m_pState->ReleaseSource();
            }

            m_pState = other.m_pState;

            if (m_pState != nullptr)
            {
                m_pState->AddSourceRef();
            }
        }

        return *this;
    }

    CancellationSource& CancellationSource::operator=(CancellationSource&& other) noexcept
    {
        if (this != &other)
        {
            if (m_pState != nullptr)
            {
                m_pState->ReleaseSource();
            }

            m_pState       = other.m_pState;
            other.m_pState = nullptr;
        }

        return *this;
    }

    bool CancellationSource::IsCancellationRequested() const noexcept
    {
        return m_pState != nullptr && m_pState->IsCancellationRequested();
    }

    bool CancellationSource::CanBeCancelled() const noexcept
    {
        return m_pState != nullptr;
    }

    CancellationToken CancellationSource::GetToken() const noexcept
    {
        return CancellationToken(m_pState);
    }

    void CancellationSource::Cancel()
    {
        if (m_pState)
        {
            m_pState->Cancel();
        }
    }
} // namespace UN::Async
