#pragma once
#include <UnAsync/Internal/WhenAllCounter.h>
#include <tuple>

namespace UN::Async::Internal
{
    template<typename TTaskContainer>
    class WhenAllReadyAwaiter;

    template<>
    class WhenAllReadyAwaiter<std::tuple<>>
    {
    public:
        inline constexpr WhenAllReadyAwaiter() noexcept {}

        inline explicit constexpr WhenAllReadyAwaiter(std::tuple<>) noexcept {}

        [[nodiscard]] constexpr bool await_ready() const noexcept
        {
            return true;
        }

        inline void await_suspend(std::coroutine_handle<>) noexcept {}

        inline std::tuple<> await_resume() const noexcept
        {
            return {};
        }
    };

    template<typename... TTasks>
    class WhenAllReadyAwaiter<std::tuple<TTasks...>>
    {
        inline bool IsReady() const noexcept
        {
            return m_Counter.IsReady();
        }

        inline bool TryAwait(std::coroutine_handle<> awaitingCoroutine) noexcept
        {
            StartTasks(std::make_integer_sequence<std::size_t, sizeof...(TTasks)>{});
            return m_Counter.TryAwait(awaitingCoroutine);
        }

        template<std::size_t... INDICES>
        inline void StartTasks(std::integer_sequence<std::size_t, INDICES...>) noexcept
        {
            (void)std::initializer_list<int>{ (std::get<INDICES>(m_Tasks).Start(m_Counter), 0)... };
        }

        WhenAllCounter m_Counter;
        std::tuple<TTasks...> m_Tasks;

    public:
        inline explicit WhenAllReadyAwaiter(TTasks&&... tasks) noexcept(
            std::conjunction_v<std::is_nothrow_move_constructible<TTasks>...>)
            : m_Counter(sizeof...(TTasks))
            , m_Tasks(std::move(tasks)...)
        {
        }

        inline explicit WhenAllReadyAwaiter(std::tuple<TTasks...>&& tasks) noexcept(
            std::is_nothrow_move_constructible_v<std::tuple<TTasks...>>)
            : m_Counter(sizeof...(TTasks))
            , m_Tasks(std::move(tasks))
        {
        }

        inline WhenAllReadyAwaiter(WhenAllReadyAwaiter&& other) noexcept
            : m_Counter(sizeof...(TTasks))
            , m_Tasks(std::move(other.m_Tasks))
        {
        }

        inline auto operator co_await() & noexcept
        {
            class Awaiter
            {
                WhenAllReadyAwaiter& m_Awaitable;

            public:
                inline Awaiter(WhenAllReadyAwaiter& awaitable) noexcept
                    : m_Awaitable(awaitable)
                {
                }

                inline bool await_ready() const noexcept
                {
                    return m_Awaitable.IsReady();
                }

                inline bool await_suspend(std::coroutine_handle<> awaitingCoroutine) noexcept
                {
                    return m_Awaitable.TryAwait(awaitingCoroutine);
                }

                inline std::tuple<TTasks...>& await_resume() noexcept
                {
                    return m_Awaitable.m_Tasks;
                }
            };

            return Awaiter{ *this };
        }

        inline auto operator co_await() && noexcept
        {
            class Awaiter
            {
                WhenAllReadyAwaiter& m_Awaitable;

            public:
                inline Awaiter(WhenAllReadyAwaiter& awaitable) noexcept
                    : m_Awaitable(awaitable)
                {
                }

                inline bool await_ready() const noexcept
                {
                    return m_Awaitable.IsReady();
                }

                inline bool await_suspend(std::coroutine_handle<> awaitingCoroutine) noexcept
                {
                    return m_Awaitable.TryAwait(awaitingCoroutine);
                }

                inline std::tuple<TTasks...>&& await_resume() noexcept
                {
                    return std::move(m_Awaitable.m_Tasks);
                }
            };

            return Awaiter{ *this };
        }
    };

    template<typename TTaskContainer>
    class WhenAllReadyAwaiter
    {
        bool IsReady() const noexcept
        {
            return m_Counter.IsReady();
        }

        bool TryAwait(std::coroutine_handle<> awaitingCoroutine) noexcept
        {
            for (auto&& task : m_Tasks)
            {
                task.Start(m_Counter);
            }

            return m_Counter.TryAwait(awaitingCoroutine);
        }

        WhenAllCounter m_Counter;
        TTaskContainer m_Tasks;

    public:
        inline explicit WhenAllReadyAwaiter(TTaskContainer&& tasks) noexcept
            : m_Counter(tasks.size())
            , m_Tasks(std::forward<TTaskContainer>(tasks))
        {
        }

        inline WhenAllReadyAwaiter(WhenAllReadyAwaiter&& other) noexcept(std::is_nothrow_move_constructible_v<TTaskContainer>)
            : m_Counter(other.m_Tasks.size())
            , m_Tasks(std::move(other.m_Tasks))
        {
        }

        inline WhenAllReadyAwaiter(const WhenAllReadyAwaiter&)            = delete;
        inline WhenAllReadyAwaiter& operator=(const WhenAllReadyAwaiter&) = delete;

        inline auto operator co_await() & noexcept
        {
            class Awaiter
            {
                WhenAllReadyAwaiter& m_Awaitable;

            public:
                inline Awaiter(WhenAllReadyAwaiter& awaitable)
                    : m_Awaitable(awaitable)
                {
                }

                inline bool await_ready() const noexcept
                {
                    return m_Awaitable.IsReady();
                }

                inline bool await_suspend(std::coroutine_handle<> awaitingCoroutine) noexcept
                {
                    return m_Awaitable.TryAwait(awaitingCoroutine);
                }

                inline TTaskContainer& await_resume() noexcept
                {
                    return m_Awaitable.m_Tasks;
                }
            };

            return Awaiter{ *this };
        }

        inline auto operator co_await() && noexcept
        {
            class Awaiter
            {
                WhenAllReadyAwaiter& m_Awaitable;

            public:
                inline Awaiter(WhenAllReadyAwaiter& awaitable)
                    : m_Awaitable(awaitable)
                {
                }

                inline bool await_ready() const noexcept
                {
                    return m_Awaitable.IsReady();
                }

                inline bool await_suspend(std::coroutine_handle<> awaitingCoroutine) noexcept
                {
                    return m_Awaitable.TryAwait(awaitingCoroutine);
                }

                inline TTaskContainer&& await_resume() noexcept
                {
                    return std::move(m_Awaitable.m_Tasks);
                }
            };

            return Awaiter{ *this };
        }
    };
} // namespace UN::Async::Internal
