#pragma once

#include <atomic>
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

    bool TryCancel() {
        auto expected = TaskStage::kPending;
        return stage.compare_exchange_strong(expected, TaskStage::kCancelled);
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
