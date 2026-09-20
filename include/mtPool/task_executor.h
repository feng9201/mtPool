#pragma once

#include "mtPool/task.h"

namespace mtPool {

// Abstracts "run this task on some executor" so the scheduler does not depend
// on BS::thread_pool directly. Immediate execution of due delayed tasks goes here.
class TaskExecutor {
public:
    virtual ~TaskExecutor() = default;
    virtual void Execute(Task task) = 0;
};

}  // namespace mtPool
