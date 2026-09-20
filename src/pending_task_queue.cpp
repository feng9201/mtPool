#include "mtPool/pending_task_queue.h"

namespace mtPool {

void PendingTaskQueue::Insert(DelayedTask task) {
    auto key = MakeScheduleKey(task);
    tasks_.emplace(key, std::move(task));
    ++active_count_;
}

std::vector<DelayedTask> PendingTaskQueue::PopDueRunnable(TimePoint now, SequenceTracker& tracker) {
    std::vector<DelayedTask> due;
    for (auto it = tasks_.begin(); it != tasks_.end();) {
        DelayedTask& candidate = it->second;
        if (candidate.cancel && candidate.cancel->IsCancelled()) {
            it = tasks_.erase(it);
            --active_count_;
            continue;
        }
        if (candidate.run_at > now) {
            break;
        }
        if (!tracker.IsRunnable(candidate.token)) {
            ++it;
            continue;
        }
        if (candidate.cancel && !candidate.cancel->TryDispatch()) {
            it = tasks_.erase(it);
            --active_count_;
            continue;
        }
        tracker.Acquire(candidate.token);
        due.push_back(std::move(candidate));
        it = tasks_.erase(it);
        --active_count_;
    }
    return due;
}

std::optional<TimePoint> PendingTaskQueue::NextWakeTime(TimePoint now) const {
    for (const auto& entry : tasks_) {
        if (entry.second.cancel && entry.second.cancel->IsCancelled()) {
            continue;
        }
        if (entry.second.run_at > now) {
            return entry.second.run_at;
        }
    }
    return std::nullopt;
}

bool PendingTaskQueue::Empty() const {
    return tasks_.empty();
}

std::size_t PendingTaskQueue::Size() const {
    return tasks_.size();
}

std::size_t PendingTaskQueue::ActiveCount() const {
    return active_count_;
}

std::vector<DelayedTask> PendingTaskQueue::TakeAll() {
    std::vector<DelayedTask> all;
    all.reserve(tasks_.size());
    for (auto& entry : tasks_) {
        all.push_back(std::move(entry.second));
    }
    tasks_.clear();
    active_count_ = 0;
    return all;
}

}  // namespace mtPool
