#include "mtPool/delayed_task_handle.h"

namespace mtPool {

DelayedTaskHandle::DelayedTaskHandle(CancelStatePtr state) : state_(std::move(state)) {}

bool DelayedTaskHandle::Cancel() {
    return state_ && state_->TryCancel();
}

bool DelayedTaskHandle::IsCancelled() const {
    return state_ && state_->IsCancelled();
}

bool DelayedTaskHandle::IsPending() const {
    return state_ && state_->stage.load() == TaskStage::kPending;
}

DelayedTaskHandle::operator bool() const {
    return static_cast<bool>(state_);
}

}  // namespace mtPool
