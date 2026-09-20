# mtPool

进程级 C++17 线程池。工作线程基于 [BS::thread_pool](https://github.com/bshoshany/thread-pool) v5.1.0（MIT，头文件自包含）。延迟任务是可选 feature：按 Chromium `SequencedWorkerPool::PostDelayedTask` 的思路，墙钟到期后丢进线程池；无 `SequenceToken` 可并行，同一 token 串行。

```cpp
#include <mtPool/MtPool.h>

mtPool::pool().detach_task([] { /* 立即异步 */ });
mtPool::delayed().PostDelayedTask([] { /* 约 5s 后在线程池里跑 */ },
                                  std::chrono::seconds(5));
```

## 架构

延迟调度和任务执行是分开的，避免「一条 loop 线程串行跑完所有回调」：

```
任意线程  PostDelayedTask(task, delay)
                │
                ▼
      DelayedScheduler（1 条调度线程）
        PendingTaskQueue 按 (run_at, sequence_num) 排序
        SequenceTracker  记录占用中的 token
                │  wait_until(下一次到期)
                │  到期且 token 可运行
                ▼
         TaskExecutor::Execute
                │
                ▼
      BS::thread_pool 工作线程（可并行）
                │
                ▼
            task()
                │
                ▼
      释放 token，唤醒调度线程
```

两个 `delay=5s` 且 **token 不同**（或都无 token）的任务，会在约 5s 时同时交给线程池。其中一个即使跑 3s，另一个仍在约 5s 开工，而不是 8s。

同一 `SequenceToken` 的任务仍保证：同一时刻最多跑一个，后一个等前一个结束（含析构可见性由「先跑完再派下一个」保证）。

### 模块

| 模块 | 职责 |
|------|------|
| `MtPool.h` | 入口：`pool()` / `delayed()` |
| `mt_thread_pool.h` | 第三方 `BS::thread_pool` |
| `thread_pool_executor` | 把到期任务 `detach_task` 进池 |
| `delayed_task_runner` | 对外 API：Post / Submit / Shutdown |
| `delayed_scheduler` | 调度线程、`wait_until`、派发 |
| `pending_task_queue` | 到期优先队列 |
| `sequence_token` / `sequence_tracker` | 序列号与运行互斥 |
| `delayed_task_handle` / `cancel_state` | 到期前取消 |
| `task_executor` | 执行器抽象，调度层不直接依赖 BS |
| `export.h` | Windows `__declspec` / ELF visibility |

调度只用标准库：`std::chrono::steady_clock`、`std::mutex`、`std::condition_variable`、`std::thread`，不绑 Windows 消息泵。

### 单例与链接

`pool()` 与 `delayed()` 都是函数内 magic static。`delayed()` 会先碰到 `pool()`，保证析构时先停调度、再拆线程池。

- 默认 **静态库**：每个链接该库的 EXE/DLL 各有一份实例。
- `MTPOOL_BUILD_SHARED=ON`：**进程内** EXE 与各 DLL 共享同一池（跨进程仍隔离）。

## 构建

CMake ≥ 3.15，C++17。

```bat
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
build\Release\mtpool_delayed_demo.exe
```

其它生成器：

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/mtpool_delayed_demo
```

选项：

| 选项 | 默认 | 说明 |
|------|------|------|
| `MTPOOL_ENABLE_DELAYED` | ON | 编译延迟任务（`delayed()` / SequenceToken）。OFF 时库只剩线程池 |
| `MTPOOL_BUILD_SHARED` | OFF | ON 则编 DLL/so，进程内 EXE/DLL 共享同一单例 |
| `MTPOOL_BUILD_DEMO` | ON | 延迟任务 demo（还需要 `MTPOOL_ENABLE_DELAYED`） |

```bat
cmake -S . -B build -DMTPOOL_ENABLE_DELAYED=ON -DMTPOOL_BUILD_SHARED=ON
```

业务工程：

```cmake
add_subdirectory(path/to/mtPool)
target_link_libraries(your_app PRIVATE mtPool)
```

```cpp
#include <mtPool/MtPool.h>
```

## vcpkg

本仓库 overlay：`ports/mtpool/`。私有 git registry：[vcpkg-registrys](https://github.com/feng9201/vcpkg-registrys)。

```bash
vcpkg install mtpool --overlay-ports=<repo>/ports
vcpkg install mtpool[shared] --overlay-ports=<repo>/ports
vcpkg install mtpool[delayed,shared] --overlay-ports=<repo>/ports
```

| feature | 作用 |
|---------|------|
| （无） | 只有线程池。链接方式跟 triplet：`x64-windows` → 动态，`x64-windows-static` → 静态 |
| `shared` | 强制动态库（即使 triplet 是 static）。多模块共用一份 `pool()` 时建议打开 |
| `delayed` | `-DMTPOOL_ENABLE_DELAYED=ON`，提供 `mtPool::delayed()` |

消费工程：

```json
{
  "dependencies": [
    {
      "name": "mtpool",
      "features": ["delayed", "shared"]
    }
  ]
}
```

`vcpkg-configuration.json` 要把 `mtpool` 指到私有 registry（`baseline` 用已包含 1.0.0 的提交）：

```json
{
  "registries": [
    {
      "kind": "git",
      "repository": "https://github.com/feng9201/vcpkg-registrys.git",
      "baseline": "bb227c143f9d71b8c42d9c8813cf2c22f6e1d9b7",
      "packages": ["mtpool"]
    }
  ]
}
```

```cmake
find_package(mtPool CONFIG REQUIRED)
target_link_libraries(your_app PRIVATE mtPool::mtPool)
```

安装时的 `delayed` / `shared` 会写进 `mtPoolConfig.cmake` 和目标的接口宏（`MTPOOL_ENABLE_DELAYED`、`MTPOOL_SHARED`）。未开 `delayed` 时不要调用 `mtPool::delayed()`。

## 使用

### 立即任务（原线程池）

```cpp
#include <mtPool/MtPool.h>

mtPool::pool().detach_task([] { doWork(); });

auto fut = mtPool::pool().submit([] { return 42; });
int v = fut.get();

mtPool::pool().reset(8);           // 调整线程数（会等待现有任务）
mtPool::pool().wait_for_tasks();   // 勿在工作线程里调用，会死锁
```

`pause` / `unpause` / `get_thread_count` 等仍走 `BS::thread_pool` 原接口。

### 延迟任务

`delay <= 0` 视为立即交给线程池（仍受 token 约束）。

```cpp
using namespace std::chrono_literals;

mtPool::delayed().PostDelayedTask([] { tick(); }, 5s);

auto fut = mtPool::delayed().SubmitDelayed([] { return 7; }, 30ms);
int n = fut.get();

mtPool::delayed().WaitUntilIdle();
```

### 取消

```cpp
auto h = mtPool::delayed().PostDelayedTask([] { fire(); }, 100ms);
if (h.Cancel()) {
    // 仍在队列里、尚未派进线程池
}
```

已经开始执行的任务取消失败，回调仍会跑完。

### SequenceToken

默认构造的 token 无效（id=0），任务之间可并行。

```cpp
auto token = mtPool::SequenceToken::Create();
mtPool::delayed().PostDelayedTask(taskA, 1s, token);
mtPool::delayed().PostDelayedTask(taskB, 1s, token);  // B 等 A 结束

// 同名永远映射到同一 id，方便跨模块共用一条队列
auto io = mtPool::SequenceToken::Named("io");
```

### 不要在工作线程里死等自己

下面会卡死（池里的线程在等池空闲）：

```cpp
mtPool::pool().detach_task([] {
    mtPool::pool().wait_for_tasks();  // 不要这样做
});
```

`WaitUntilIdle()` / `Shutdown()` 在延迟任务回调里调用会抛 `std::logic_error`（fail fast），不会像上面那样卡死。

## 行为要点

1. **到期时间**是投递时的 `steady_clock::now() + delay`，允许毫秒级抖动（调度唤醒、线程切换）。
2. **无 token**：到期后并行进池，一个任务耗时长不会重新计算另一个的 delay。
3. **有 token**：到期后若该序列仍被占用，任务留在队列里，等前一个释放再派发。
4. 调度线程本身不跑业务回调，只负责睡到点、再 `detach_task`。
5. **Shutdown**（含进程退出时静态析构）：未到期任务取消并销毁——handle 变 `IsCancelled`，`SubmitDelayed` 的 future 抛 `std::future_errc::broken_promise`；已派发到池里的任务会等跑完。
6. **Cancel** 一个还没派发的任务会立刻唤醒调度线程把它丢掉，不会等到原定到期时间；若是 `SubmitDelayed` 的任务，future 同样立刻 `broken_promise`。
7. 在延迟回调里调 `Shutdown()` / `WaitUntilIdle()` 会立即抛 `std::logic_error`（debug 下同时 assert，stderr 有日志），不会死锁。

## Demo

`demo/delayed_task_demo.cpp` 覆盖：并行到期、同 token 串行、Cancel、`SubmitDelayed`、Named token、取消后 future `broken_promise`、回调内自等检测。Release 下应全部 PASS。
