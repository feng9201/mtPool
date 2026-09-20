#pragma once
/*****
* @brief: 进程级单例线程池，自包含 BS::thread_pool v5.1.0 (MIT)
* @auth:  hxf
*
*   #include <mtPool/MtPool.h>
*
*   mtPool::pool().detach_task([] { doWork(); });
*   auto fut = mtPool::pool().submit([] { return 42; });
*
*   // 延迟任务需要 MTPOOL_ENABLE_DELAYED（CMake / vcpkg feature delayed）
*   mtPool::delayed().PostDelayedTask([] {}, std::chrono::seconds(5));
******/

#include "mtPool/export.h"
#include "mtPool/mt_thread_pool.h"

#if defined(MTPOOL_ENABLE_DELAYED)
#include "mtPool/delayed_task_runner.h"
#include "mtPool/sequence_token.h"
#endif

namespace mtPool
{
	MTPOOL_API BS::thread_pool<>& pool();

#if defined(MTPOOL_ENABLE_DELAYED)
	MTPOOL_API DelayedTaskRunner& delayed();
#endif
}
