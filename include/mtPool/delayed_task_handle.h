#pragma once

#include "mtPool/cancel_state.h"
#include "mtPool/export.h"

namespace mtPool {

class MTPOOL_API DelayedTaskHandle {
public:
    DelayedTaskHandle() = default;
    explicit DelayedTaskHandle(CancelStatePtr state);

    // Returns true if the task was still pending and is now cancelled.
    bool Cancel();

    bool IsCancelled() const;
    bool IsPending() const;

    explicit operator bool() const;

private:
    CancelStatePtr state_;
};

}  // namespace mtPool
