#include "mtPool/delayed_task_runner.h"

#include "mtPool/delayed_scheduler.h"

namespace mtPool {

DelayedTaskRunner::DelayedTaskRunner(TaskExecutor& executor)
    : scheduler_(std::make_unique<DelayedScheduler>(executor)) {}

DelayedTaskRunner::~DelayedTaskRunner() {
    Shutdown();
}

DelayedTaskHandle DelayedTaskRunner::PostTask(Task task, SequenceToken token) {
    return PostDelayedTask(std::move(task), Duration::zero(), token);
}

DelayedTaskHandle DelayedTaskRunner::PostDelayedTask(Task task,
                                                     Duration delay,
                                                     SequenceToken token) {
    return scheduler_->Post(std::move(task), delay, token);
}

void DelayedTaskRunner::Shutdown() {
    if (scheduler_) {
        scheduler_->Shutdown();
    }
}

void DelayedTaskRunner::WaitUntilIdle() {
    if (scheduler_) {
        scheduler_->WaitUntilIdle();
    }
}

std::size_t DelayedTaskRunner::PendingCount() const {
    return scheduler_ ? scheduler_->PendingCount() : 0;
}

std::size_t DelayedTaskRunner::InFlightCount() const {
    return scheduler_ ? scheduler_->InFlightCount() : 0;
}

}  // namespace mtPool
