#pragma once

#include <atomic>
#include <condition_variable>
#include <memory>

namespace mtPool {

enum class TaskStage {
    kPending,
    kDispatched,
    kCancelled,
    kFinished,
};

struct CancelState {
    std::atomic<TaskStage> stage{TaskStage::kPending};

    // Set by the scheduler before the task is published. Wakes the scheduler
    // (and WaitUntilIdle) when Cancel() wins the race. weak_ptr so a handle
    // that outlives the scheduler can never touch a dead condition_variable.
    std::weak_ptr<std::condition_variable> wake;

    bool TryCancel() {
        auto expected = TaskStage::kPending;
        if (!stage.compare_exchange_strong(expected, TaskStage::kCancelled)) {
            return false;
        }
        if (auto cv = wake.lock()) {
            cv->notify_all();
        }
        return true;
    }

    bool IsCancelled() const {
        return stage.load() == TaskStage::kCancelled;
    }

    bool TryDispatch() {
        auto expected = TaskStage::kPending;
        return stage.compare_exchange_strong(expected, TaskStage::kDispatched);
    }

    void MarkFinished() {
        stage.store(TaskStage::kFinished);
    }
};

using CancelStatePtr = std::shared_ptr<CancelState>;

}  // namespace mtPool
