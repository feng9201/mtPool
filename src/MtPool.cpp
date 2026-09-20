#include "mtPool/MtPool.h"

#if defined(MTPOOL_ENABLE_DELAYED)
#include "mtPool/thread_pool_executor.h"
#endif

namespace mtPool
{
	BS::thread_pool<>& pool()
	{
		static BS::thread_pool<> instance;
		return instance;
	}

#if defined(MTPOOL_ENABLE_DELAYED)
	DelayedTaskRunner& delayed()
	{
		(void)pool();
		static ThreadPoolExecutor executor;
		static DelayedTaskRunner runner(executor);
		return runner;
	}
#endif
}
