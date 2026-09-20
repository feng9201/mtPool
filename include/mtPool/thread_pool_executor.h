#pragma once

#include "mtPool/export.h"
#include "mtPool/task.h"
#include "mtPool/task_executor.h"

namespace mtPool {

class MTPOOL_API ThreadPoolExecutor : public TaskExecutor {
public:
    void Execute(Task task) override;
};

}  // namespace mtPool
