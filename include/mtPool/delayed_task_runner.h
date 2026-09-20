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

    void Shutdown();
    void WaitUntilIdle();

    std::size_t PendingCount() const;
    std::size_t InFlightCount() const;

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
