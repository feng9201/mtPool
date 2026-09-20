#pragma once

#include "mtPool/clock.h"
#include "mtPool/delayed_task_handle.h"
#include "mtPool/export.h"
#include "mtPool/sequence_token.h"
#include "mtPool/task.h"
#include "mtPool/task_executor.h"

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <thread>

namespace mtPool {

class PendingTaskQueue;
class SequenceTracker;

// Dedicated coordinator thread: wait until the next time_to_run, then hand
// due tasks to TaskExecutor (the worker pool). Workers do the real work so
// two independent delayed tasks can start at ~the same wall time.
class MTPOOL_API DelayedScheduler {
public:
    explicit DelayedScheduler(TaskExecutor& executor);
    ~DelayedScheduler();

    DelayedScheduler(const DelayedScheduler&) = delete;
    DelayedScheduler& operator=(const DelayedScheduler&) = delete;

    DelayedTaskHandle Post(Task task, Duration delay, SequenceToken token);

    void NotifySequenceReleased();
    void Shutdown();
    void WaitUntilIdle();

    std::size_t PendingCount() const;
    std::size_t ActiveCount() const;
    std::size_t InFlightCount() const;

private:
    void ThreadMain();
    void DispatchDueTasks();
    void WaitForInFlight();

    TaskExecutor& executor_;
    std::unique_ptr<PendingTaskQueue> queue_;
    std::unique_ptr<SequenceTracker> tracker_;

    mutable std::mutex mutex_;
    // shared_ptr so CancelState can hold a weak_ptr and wake us even if the
    // handle outlives this scheduler.
    std::shared_ptr<std::condition_variable> cv_;
    std::atomic<bool> stop_{false};
    std::uint64_t next_sequence_num_ = 0;
    std::atomic<std::size_t> in_flight_{0};
    std::thread thread_;
};

}  // namespace mtPool
