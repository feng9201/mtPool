#pragma once

#include "mtPool/clock.h"
#include "mtPool/delayed_task.h"
#include "mtPool/sequence_tracker.h"

#include <cstddef>
#include <map>
#include <optional>
#include <vector>

namespace mtPool {

// Pending delayed tasks sorted by (run_at, sequence_num), same idea as
// SequencedWorkerPool's pending_tasks_ set.
class PendingTaskQueue {
public:
    void Insert(DelayedTask task);

    // Pop every due, not-cancelled, sequence-runnable task.
    // Cancelled due tasks are discarded.
    // Due but sequence-blocked tasks stay in the queue.
    std::vector<DelayedTask> PopDueRunnable(TimePoint now, SequenceTracker& tracker);

    // Earliest wake-up for still-pending, not-yet-due tasks.
    // nullopt: nothing time-based to wait for (idle, or only sequence-blocked).
    std::optional<TimePoint> NextWakeTime(TimePoint now) const;

    bool Empty() const;
    std::size_t Size() const;
    std::size_t ActiveCount() const;

private:
    std::map<ScheduleKey, DelayedTask> tasks_;
};

}  // namespace mtPool
