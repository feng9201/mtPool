#include "mtPool/thread_pool_executor.h"

#include "mtPool/MtPool.h"

namespace mtPool {

void ThreadPoolExecutor::Execute(Task task) {
    pool().detach_task(std::move(task));
}

}  // namespace mtPool
