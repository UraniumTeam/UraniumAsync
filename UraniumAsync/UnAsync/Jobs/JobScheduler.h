#pragma once
#include <UnAsync/Jobs/IJobScheduler.h>
#include <UnAsync/Jobs/Job.h>
#include <UnAsync/Parallel/Semaphore.h>
#include <UnTL/Containers/List.h>
#include <deque>
#include <mutex>
#include <shared_mutex>
#include <thread>

namespace UN::Async
{
    class JobGlobalQueue
    {
        using DequeAllocator = StdHeapAllocator<Job*>;
        std::deque<Job*, DequeAllocator> m_Deque;
        std::mutex m_Mutex;

    public:
        inline bool Empty();
        inline void Enqueue(Job* job);
        inline Job* Dequeue();
    };

    bool JobGlobalQueue::Empty()
    {
        std::unique_lock lk(m_Mutex);
        return m_Deque.empty();
    }

    void JobGlobalQueue::Enqueue(Job* job)
    {
        std::unique_lock lk(m_Mutex);
        constexpr auto compare = [](Job* lhs, Job* rhs) {
            return lhs->GetPriority() > rhs->GetPriority();
        };

        auto it = std::lower_bound(m_Deque.begin(), m_Deque.end(), job, compare);
        m_Deque.insert(it, job);
    }

    Job* JobGlobalQueue::Dequeue()
    {
        std::unique_lock lk(m_Mutex);
        if (!m_Deque.empty())
        {
            auto* t = m_Deque.front();
            m_Deque.pop_front();
            return t;
        }
        return nullptr;
    }

    class JobWorkerQueue
    {
        using DequeAllocator = StdHeapAllocator<Job*>;
        std::deque<Job*, DequeAllocator> m_Deque;
        std::mutex m_Mutex;

        inline Job* GetFrontNoLock();

    public:
        inline void Enqueue(Job* job);
        inline Job* SelfSteal();
        inline Job* Steal();
    };

    Job* JobWorkerQueue::GetFrontNoLock()
    {
        if (!m_Deque.empty())
        {
            auto* front = m_Deque.front();
            m_Deque.pop_front();
            return front;
        }

        return nullptr;
    }

    void JobWorkerQueue::Enqueue(Job* job)
    {
        std::unique_lock lk(m_Mutex);
        constexpr auto compare = [](Job* lhs, Job* rhs) {
            return lhs->GetPriority() > rhs->GetPriority();
        };

        auto it = std::lower_bound(m_Deque.begin(), m_Deque.end(), job, compare);
        m_Deque.insert(it, job);
    }

    Job* JobWorkerQueue::SelfSteal()
    {
        std::unique_lock lk(m_Mutex);
        return GetFrontNoLock();
    }

    Job* JobWorkerQueue::Steal()
    {
        auto pauseCount = 1;
        for (Int32 i = 0; i < 16; ++i)
        {
            if (m_Mutex.try_lock())
            {
                auto* front = GetFrontNoLock();
                m_Mutex.unlock();
                return front;
            }
            if (pauseCount > 32)
            {
                std::this_thread::yield();
                continue;
            }
            for (Int32 j = 0; j < pauseCount; ++j)
            {
                _mm_pause();
            }
            pauseCount <<= 1;
        }
        return nullptr;
    }

    struct SchedulerThreadInfo
    {
        std::thread Thread;
        JobWorkerQueue Queue;
        UInt32 WorkerID = static_cast<UInt32>(-1);
        std::thread::id ThreadID;
        Semaphore WaitSemaphore;
        std::atomic_bool IsSleeping;

        [[nodiscard]] inline bool IsWorker() const noexcept
        {
            return WorkerID != static_cast<UInt32>(-1);
        }
    };

    class JobScheduler final : public Object<IJobScheduler>
    {
        const UInt32 m_WorkerCount;
        List<SchedulerThreadInfo*> m_Threads;
        std::shared_mutex m_ThreadsMutex;
        JobGlobalQueue m_GlobalQueue;

        Semaphore m_Semaphore;
        std::atomic<Int32> m_SleepingWorkerCount;
        std::atomic_bool m_ShouldExit;

        static thread_local SchedulerThreadInfo* m_CurrentThreadInfo;
        inline static constexpr UInt32 MaxThreadCount = 32;

        void NotifyWorker();
        void WorkerThreadProcess(UInt32 id);
        void ProcessJobs();
        void Execute(Job* job);
        SchedulerThreadInfo* GetCurrentThread();
        Job* TryStealJob(UInt32& victimIndex);

    public:
        UN_RTTI_Class(JobScheduler, "6754DA31-46FA-4661-A46E-2787E6D9FD29");

        explicit JobScheduler(UInt32 workerCount);
        ~JobScheduler() noexcept override;

        [[nodiscard]] UInt32 GetWorkerCount() const override;
        [[nodiscard]] UInt32 GetWorkerID() const override;

        void ScheduleJob(Job* job) override;
    };
} // namespace UN::Async
