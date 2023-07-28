#pragma once
#include <UnTL/Memory/Memory.h>

namespace UN::Async
{
    class Job;

    class IJobScheduler : public IObject
    {
    public:
        UN_RTTI_Class(IJobScheduler, "F9FB743A-B543-4B64-A36B-B055434DE90B");

        [[nodiscard]] virtual UInt32 GetWorkerCount() const = 0;
        [[nodiscard]] virtual UInt32 GetWorkerID() const    = 0;

        virtual void ScheduleJob(Job* job) = 0;
    };
} // namespace UN::Async
