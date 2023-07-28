#pragma once
#include <UnAsync/Jobs/IJobScheduler.h>
#include <UnAsync/Jobs/JobTree.h>
#include <UnAsync/Task.h>
#include <UnTL/Memory/Memory.h>
#include <coroutine>

namespace UN::Async
{
    class SchedulerOperation;

    //! \brief Priority of a job: from Low to Highest.
    //!
    //! \note Every value of this enum must fit in 2 bits.
    enum class JobPriority
    {
        Low,
        Normal,
        High,
        Highest
    };

    struct JobExecutionContext
    {
        UN_RTTI_Struct(JobExecutionContext, "F1295370-E5FC-4D4B-B657-7A0158F2D22C");

        UInt32 WorkerID;
    };

    //! \brief A unit of work that can be processed quickly on one thread.
    //!
    //! Multiple jobs can be scheduled and processed in parallel. If a job depends on results of other jobs,
    //! it can be set as dependent of these jobs. The job stores its dependency counter which decreases when
    //! a parent job completes. When the counter reaches zero, the job is pushed onto the global queue.
    //!
    //! The default value of dependency counter is 1, so the job won't start immediately, but will wait for
    //! a call to `Schedule()` that will decrement the counter.
    class Job
    {
    protected:
        BoolPointer<JobTree> m_TreeEmptyPair;
        Job* m_Dependent = nullptr;
        IJobScheduler* m_pScheduler;

        inline Int16 IncrementDependencyCount();
        inline Int16 DecrementDependencyCount();

    private:
        std::atomic<Int16> m_Flags{};

        inline static constexpr Int16 PriorityBitCount        = 2;
        inline static constexpr Int16 DependencyCountBitCount = 16 - PriorityBitCount;

        inline static constexpr Int16 DependencyCountShift = 0;
        inline static constexpr Int16 PriorityShift        = DependencyCountShift;
        inline static constexpr Int16 StateShift           = PriorityShift + PriorityBitCount;

        inline static constexpr Int16 DependencyCountMask = MakeMask(DependencyCountBitCount, DependencyCountShift);
        inline static constexpr Int16 PriorityMask        = MakeMask(PriorityBitCount, PriorityShift);

        //! \brief Synchronously run the job. Will be called from worker thread.
        //!
        //! \param context - The job's context used for execution.
        virtual void Execute(const JobExecutionContext& context) = 0;

    public:
        UN_RTTI_Class(Job, "69DA12B5-DFFC-4A38-BBB8-0018699C30BA");

        inline explicit Job(JobPriority priority = JobPriority::Normal, bool isEmpty = false);
        ~Job() = default;

        inline void ExecuteInternal(const JobExecutionContext& context);

        //! \brief Attach job to a JobTree node.
        //!
        //! \param tree - The node of JobTree to attach the job to.
        inline void AttachToTree(JobTree* tree);

        //! \brief Add a parent job to depend on it.
        //!
        //! This function will increment dependency counter and the parent will decrement it when it completes.
        //! A job can have multiple parents and won't be scheduled until all the parents signaled they're done.
        //!
        //! \param parent - Parent job to attach.
        inline void AttachParent(Job* parent);

        //! \brief Schedule the job to a scheduler.
        //!
        //! This function will decrement dependency counter. If job doesn't have dependencies, it will be scheduled
        //! immediately. If however there are uncompleted parent jobs, actual scheduling will occur on scheduler
        //! only when dependency counter reaches zero.
        //!
        //! \param pScheduler - Job scheduler.
        inline void Schedule(IJobScheduler* pScheduler);

        //! \brief Schedule current coroutine to a scheduler.
        //!
        //! This function creates a SchedulerOperation job that is an awaitable type.
        //! The actual scheduling will only be done when the operation is awaited.
        //!
        //! \param pScheduler - Job scheduler.
        inline static SchedulerOperation Run(IJobScheduler* pScheduler);

        template<class TFunc, class... Args>
        inline static auto Run(IJobScheduler* pScheduler, TFunc f, Args&&... args) -> std::invoke_result_t<TFunc, Args...>
        {
            co_await Run(pScheduler);

            if constexpr (std::is_void_v<std::invoke_result_t<TFunc, Args...>>)
            {
                f(std::forward<Args>(args)...);
                co_return;
            }
            else
            {
                co_return f(std::forward<Args>(args)...);
            }
        }

        //! \return Job's priority.
        [[nodiscard]] inline JobPriority GetPriority() const;

        //! \brief Set job's priority.
        //!
        //! \param priority - Priority to set for this job.
        inline void SetPriority(JobPriority priority);

        [[nodiscard]] inline bool Empty() const;

        //! \return Maximum number of dependencies a job can handle.
        [[nodiscard]] static inline constexpr UInt16 GetMaxDependencyCount()
        {
            return DependencyCountMask >> DependencyCountShift;
        }
    };

    Job::Job(JobPriority priority, bool isEmpty)
        : m_TreeEmptyPair(nullptr, isEmpty)
    {
        Int16 value = static_cast<Int16>(priority) << PriorityShift;
        value |= 1 << DependencyCountShift;
        m_Flags.store(value, std::memory_order_relaxed);
    }

    void Job::AttachToTree(JobTree* tree)
    {
        m_TreeEmptyPair.SetPointer(tree);
    }

    void Job::AttachParent(Job* parent)
    {
        parent->m_Dependent = this;
        IncrementDependencyCount();
    }

    void Job::SetPriority(JobPriority priority)
    {
        Int16 cleared = m_Flags.load() & ~PriorityMask;
        Int16 value   = static_cast<Int16>(priority) << PriorityShift;
        m_Flags.store(value | cleared);
    }

    JobPriority Job::GetPriority() const
    {
        Int16 value = m_Flags.load() & PriorityMask;
        return static_cast<JobPriority>(value >> PriorityShift);
    }

    void Job::Schedule(IJobScheduler* pScheduler)
    {
        m_pScheduler = pScheduler;
        DecrementDependencyCount();
    }

    Int16 Job::IncrementDependencyCount()
    {
        return (++m_Flags & DependencyCountMask) >> DependencyCountShift;
    }

    Int16 Job::DecrementDependencyCount()
    {
        auto value = (--m_Flags & DependencyCountMask) >> DependencyCountShift;
        if (value == 0)
        {
            m_pScheduler->ScheduleJob(this);
        }

        return static_cast<Int16>(value);
    }

    void Job::ExecuteInternal(const JobExecutionContext& context)
    {
        auto* dependent = m_Dependent;
        Execute(context);
        if (dependent)
        {
            dependent->DecrementDependencyCount();
        }
    }

    bool Job::Empty() const
    {
        return m_TreeEmptyPair.GetBool();
    }

    class SchedulerOperation : public Job
    {
        std::coroutine_handle<> m_AwaitingCoroutine;

        inline void Execute(const JobExecutionContext&) override
        {
            m_AwaitingCoroutine.resume();
        }

    public:
        inline explicit SchedulerOperation(IJobScheduler* pScheduler) noexcept
            : Job()
        {
            m_pScheduler = pScheduler;
        }

        [[nodiscard]] inline bool await_ready() noexcept
        {
            return false;
        }

        inline void await_suspend(std::coroutine_handle<> awaitingCoroutine) noexcept
        {
            m_AwaitingCoroutine = awaitingCoroutine;
            DecrementDependencyCount();
        }

        inline void await_resume() noexcept {}
    };

    SchedulerOperation Job::Run(IJobScheduler* pScheduler)
    {
        return SchedulerOperation(pScheduler);
    }
} // namespace UN::Async
