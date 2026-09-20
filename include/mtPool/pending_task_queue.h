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

    // O(1)：计数随 Insert/erase 维护。已取消但尚未被调度线程清掉的任务
    // 仍算在内（清掉时才减），调用方靠 cv 唤醒后重新检查。
    std::size_t ActiveCount() const;

    // Move every pending task out and clear the queue (used by Shutdown).
    std::vector<DelayedTask> TakeAll();

private:
    std::map<ScheduleKey, DelayedTask> tasks_;
    std::size_t active_count_ = 0;
};

}  // namespace mtPool
