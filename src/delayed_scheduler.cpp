#include "mtPool/delayed_scheduler.h"

#include "mtPool/pending_task_queue.h"
#include "mtPool/sequence_tracker.h"

#include <chrono>
#include <utility>
#include <vector>

namespace mtPool {

DelayedScheduler::DelayedScheduler(TaskExecutor& executor)
    : executor_(executor),
      queue_(std::make_unique<PendingTaskQueue>()),
      tracker_(std::make_unique<SequenceTracker>()) {
    thread_ = std::thread([this] { ThreadMain(); });
}

DelayedScheduler::~DelayedScheduler() {
    Shutdown();
}

DelayedTaskHandle DelayedScheduler::Post(Task task, Duration delay, SequenceToken token) {
    if (!task) {
        return DelayedTaskHandle();
    }

    DelayedTask item;
    item.task = std::move(task);
    item.token = token;
    item.cancel = std::make_shared<CancelState>();
    item.run_at = (delay > Duration::zero()) ? (Clock::now() + delay) : Clock::now();

    DelayedTaskHandle handle(item.cancel);
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (stop_.load()) {
            item.cancel->TryCancel();
            return DelayedTaskHandle();
        }
        item.sequence_num = next_sequence_num_++;
        queue_->Insert(std::move(item));
    }
    cv_.notify_one();
    return handle;
}

void DelayedScheduler::NotifySequenceReleased() {
    cv_.notify_one();
}

void DelayedScheduler::Shutdown() {
    bool expected = false;
    if (!stop_.compare_exchange_strong(expected, true)) {
        if (thread_.joinable()) {
            thread_.join();
        }
        WaitForInFlight();
        return;
    }
    cv_.notify_all();
    if (thread_.joinable()) {
        thread_.join();
    }
    WaitForInFlight();
}

void DelayedScheduler::WaitUntilIdle() {
    std::unique_lock<std::mutex> lock(mutex_);
    while (queue_->ActiveCount() != 0 || in_flight_.load() != 0) {
        cv_.wait_for(lock, std::chrono::milliseconds(5));
    }
}

std::size_t DelayedScheduler::PendingCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_->Size();
}

std::size_t DelayedScheduler::ActiveCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_->ActiveCount();
}

std::size_t DelayedScheduler::InFlightCount() const {
    return in_flight_.load();
}

void DelayedScheduler::ThreadMain() {
    std::unique_lock<std::mutex> lock(mutex_);
    while (!stop_.load()) {
        lock.unlock();
        DispatchDueTasks();
        lock.lock();
        if (stop_.load()) {
            break;
        }

        // Sequence-blocked due tasks stay in the queue. Do not treat
        // "queue not empty" as runnable, or the wait would spin.
        const TimePoint now = Clock::now();
        const auto wake = queue_->NextWakeTime(now);
        if (!wake.has_value()) {
            cv_.wait(lock);
        } else {
            cv_.wait_until(lock, *wake);
        }
    }
}

void DelayedScheduler::DispatchDueTasks() {
    std::vector<DelayedTask> due;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        due = queue_->PopDueRunnable(Clock::now(), *tracker_);
        in_flight_.fetch_add(due.size());
    }

    for (auto& item : due) {
        SequenceToken token = item.token;
        CancelStatePtr cancel = item.cancel;
        Task task = std::move(item.task);
        executor_.Execute([this, token, cancel, task = std::move(task)]() mutable {
            try {
                if (task) {
                    task();
                }
            } catch (...) {
            }
            if (cancel) {
                cancel->MarkFinished();
            }
            {
                std::lock_guard<std::mutex> lock(mutex_);
                tracker_->Release(token);
                in_flight_.fetch_sub(1);
            }
            cv_.notify_all();
        });
    }
}

void DelayedScheduler::WaitForInFlight() {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait(lock, [this] { return in_flight_.load() == 0; });
}

}  // namespace mtPool
