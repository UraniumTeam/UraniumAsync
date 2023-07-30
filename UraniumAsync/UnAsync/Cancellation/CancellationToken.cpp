#include <UnAsync/Cancellation/CancellationToken.h>
#include <UnAsync/Internal/CancellationState.h>

namespace UN::Async
{
    CancellationToken::CancellationToken() noexcept
        : m_pState(nullptr)
    {
    }

    CancellationToken::CancellationToken(const CancellationToken& other) noexcept
        : m_pState(other.m_pState)
    {
        if (m_pState)
        {
            m_pState->AddTokenRef();
        }
    }

    CancellationToken::CancellationToken(CancellationToken&& other) noexcept
        : m_pState(other.m_pState)
    {
        other.m_pState = nullptr;
    }

    CancellationToken::~CancellationToken()
    {
        if (m_pState)
        {
            m_pState->ReleaseToken();
        }
    }

    CancellationToken& CancellationToken::operator=(const CancellationToken& other) noexcept
    {
        if (m_pState != other.m_pState)
        {
            if (m_pState != nullptr)
            {
                m_pState->ReleaseToken();
            }

            m_pState = other.m_pState;

            if (m_pState != nullptr)
            {
                m_pState->AddTokenRef();
            }
        }

        return *this;
    }

    CancellationToken& CancellationToken::operator=(CancellationToken&& other) noexcept
    {
        if (this != &other)
        {
            if (m_pState != nullptr)
            {
                m_pState->ReleaseToken();
            }

            m_pState       = other.m_pState;
            other.m_pState = nullptr;
        }

        return *this;
    }

    bool CancellationToken::IsCancellationRequested() const noexcept
    {
        return m_pState != nullptr && m_pState->IsCancellationRequested();
    }

    bool CancellationToken::CanBeCancelled() const noexcept
    {
        return m_pState != nullptr && m_pState->CanBeCancelled();
    }

    CancellationToken::CancellationToken(Internal::CancellationState* pState) noexcept
        : m_pState(pState)
    {
        if (m_pState)
        {
            m_pState->AddTokenRef();
        }
    }
} // namespace UN::Async
