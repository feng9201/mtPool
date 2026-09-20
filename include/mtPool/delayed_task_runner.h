#pragma once

#include "mtPool/clock.h"
#include "mtPool/delayed_task_handle.h"
#include "mtPool/export.h"
#include "mtPool/sequence_token.h"
#include "mtPool/task.h"
#include "mtPool/task_executor.h"

#include <functional>
#include <future>
#include <memory>
#include <type_traits>
#include <utility>

namespace mtPool {

class DelayedScheduler;

// Public facade over DelayedScheduler, modeled after
// SequencedWorkerPool::PostDelayedTask / PostDelayedSequencedWorkerTask.
class MTPOOL_API DelayedTaskRunner {
public:
    explicit DelayedTaskRunner(TaskExecutor& executor);
    ~DelayedTaskRunner();

    DelayedTaskRunner(const DelayedTaskRunner&) = delete;
    DelayedTaskRunner& operator=(const DelayedTaskRunner&) = delete;

    DelayedTaskHandle PostTask(Task task, SequenceToken token = SequenceToken());

    DelayedTaskHandle PostDelayedTask(Task task,
                                      Duration delay,
                                      SequenceToken token = SequenceToken());

    // SKIP_ON_SHUTDOWN semantics (same as SequencedWorkerPool delayed tasks):
    // pending tasks are cancelled and discarded, tasks already running on the
    // pool are awaited. Throws std::logic_error if called from a delayed task
    // callback (it would deadlock waiting for the caller itself).
    void Shutdown();

    // Blocks until the queue is empty and nothing is running. Throws
    // std::logic_error if called from a delayed task callback.
    void WaitUntilIdle();

    std::size_t PendingCount() const;
    std::size_t InFlightCount() const;

    // If the task is cancelled or discarded by Shutdown before it runs, the
    // future becomes ready with std::future_errc::broken_promise.
    template <typename F>
    auto SubmitDelayed(F&& func, Duration delay, SequenceToken token = SequenceToken())
        -> std::future<std::invoke_result_t<std::decay_t<F>>> {
        using R = std::invoke_result_t<std::decay_t<F>>;
        auto promise = std::make_shared<std::promise<R>>();
        auto future = promise->get_future();
        PostDelayedTask(
            [promise, func = std::forward<F>(func)]() mutable {
                try {
                    if constexpr (std::is_void_v<R>) {
                        func();
                        promise->set_value();
                    } else {
                        promise->set_value(func());
                    }
                } catch (...) {
                    promise->set_exception(std::current_exception());
                }
            },
            delay,
            token);
        return future;
    }

private:
    std::unique_ptr<DelayedScheduler> scheduler_;
};

}  // namespace mtPool
