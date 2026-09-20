#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <future>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <vector>

#include "mtPool/MtPool.h"

using Clock = mtPool::Clock;
using namespace std::chrono_literals;

namespace {

int g_failures = 0;

void Expect(bool cond, const char* name) {
    if (cond) {
        std::cout << "  [PASS] " << name << "\n";
    } else {
        std::cout << "  [FAIL] " << name << "\n";
        ++g_failures;
    }
}

std::int64_t MsSince(Clock::time_point start) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - start).count();
}

void DemoParallelDelayed() {
    std::cout << "\n== 1. 两个相同延迟应并行开火，不被长任务拖住 ==\n";
    mtPool::pool().reset(4);

    const auto start = Clock::now();
    std::atomic<std::int64_t> start_a{-1};
    std::atomic<std::int64_t> start_b{-1};

    mtPool::delayed().PostDelayedTask(
        [&] {
            start_a.store(MsSince(start));
            std::this_thread::sleep_for(80ms);
        },
        120ms);
    mtPool::delayed().PostDelayedTask(
        [&] { start_b.store(MsSince(start)); },
        120ms);

    mtPool::delayed().WaitUntilIdle();

    const auto a = start_a.load();
    const auto b = start_b.load();
    std::cout << "  A start = " << a << " ms, B start = " << b << " ms\n";
    Expect(a >= 120 && b >= 120, "都不早于 120ms");
    Expect(b < 180, "B 没有等到 A 的 80ms 执行完（约 200ms）");
}

void DemoSequenceTokenSerial() {
    std::cout << "\n== 2. 同一 SequenceToken 必须串行 ==\n";
    auto token = mtPool::SequenceToken::Create();
    std::atomic<std::int64_t> start_b{-1};
    std::atomic<bool> a_done{false};
    const auto start = Clock::now();

    mtPool::delayed().PostDelayedTask(
        [&] {
            std::this_thread::sleep_for(60ms);
            a_done.store(true);
        },
        40ms,
        token);
    mtPool::delayed().PostDelayedTask(
        [&] {
            start_b.store(MsSince(start));
            Expect(a_done.load(), "B 开始时 A 已结束");
        },
        40ms,
        token);

    mtPool::delayed().WaitUntilIdle();
    std::cout << "  sequenced B start = " << start_b.load() << " ms\n";
    Expect(start_b.load() >= 100, "B 大约在 40ms + A 的 60ms 之后");
}

void DemoCancel() {
    std::cout << "\n== 3. 到期前 Cancel ==\n";
    std::atomic<bool> ran{false};
    auto handle = mtPool::delayed().PostDelayedTask([&] { ran.store(true); }, 80ms);
    Expect(handle.Cancel(), "Cancel 成功");
    std::this_thread::sleep_for(120ms);
    Expect(!ran.load(), "已取消的任务未执行");
}

void DemoSubmitFuture() {
    std::cout << "\n== 4. SubmitDelayed 返回 future ==\n";
    auto fut = mtPool::delayed().SubmitDelayed([] { return 7; }, 30ms);
    Expect(fut.get() == 7, "future 得到 7");
}

void DemoNamedToken() {
    std::cout << "\n== 5. Named token 相同名字同一 id ==\n";
    auto a = mtPool::SequenceToken::Named("io");
    auto b = mtPool::SequenceToken::Named("io");
    auto c = mtPool::SequenceToken::Named("net");
    Expect(a.Equals(b), "同名 token 相等");
    Expect(!a.Equals(c), "异名 token 不等");
}

void DemoCancelBreaksPromise() {
    std::cout << "\n== 6. 取消后 promise 立即 broken，future 不干等 ==\n";
    auto promise = std::make_shared<std::promise<int>>();
    auto fut = promise->get_future();
    auto handle = mtPool::delayed().PostDelayedTask(
        [promise] { promise->set_value(1); }, 5s);
    promise.reset();  // 只留任务里的引用，任务被丢弃时 promise 才会析构
    Expect(handle.Cancel(), "Cancel 成功");

    bool broken = false;
    try {
        (void)fut.get();
    } catch (const std::future_error& e) {
        broken = (e.code() == std::future_errc::broken_promise);
    }
    Expect(broken, "future 抛 broken_promise（任务已被调度线程丢弃）");
}

void DemoSelfWaitDetected() {
    std::cout << "\n== 7. 回调里调 WaitUntilIdle/Shutdown 不死锁，抛 logic_error ==\n";
    std::atomic<bool> wait_detected{false};
    std::atomic<bool> shutdown_detected{false};
    mtPool::delayed().PostDelayedTask(
        [&] {
            try {
                mtPool::delayed().WaitUntilIdle();  // 自等，必须 fail fast
            } catch (const std::logic_error&) {
                wait_detected.store(true);
            }
            try {
                mtPool::delayed().Shutdown();  // 同上
            } catch (const std::logic_error&) {
                shutdown_detected.store(true);
            }
        },
        20ms);

    mtPool::delayed().WaitUntilIdle();  // 主线程等；若死锁这里不返回
    Expect(wait_detected.load(), "回调里 WaitUntilIdle 抛 logic_error");
    Expect(shutdown_detected.load(), "回调里 Shutdown 抛 logic_error");
}

void DemoCancelWakesWaitUntilIdle() {
    std::cout << "\n== 8. WaitUntilIdle 等待中 Cancel 能及时唤醒返回 ==\n";
    auto handle = mtPool::delayed().PostDelayedTask([] { /* 不会跑 */ }, 5s);

    std::atomic<bool> idle_returned{false};
    std::thread waiter([&] {
        mtPool::delayed().WaitUntilIdle();
        idle_returned.store(true);
    });
    std::this_thread::sleep_for(50ms);  // 确保 waiter 已在条件等待里

    Expect(handle.Cancel(), "Cancel 成功");
    waiter.join();  // Cancel 唤醒链路断了的话，这里会等到 5s 到期
    Expect(idle_returned.load(), "Cancel 后 WaitUntilIdle 及时返回");
}

}  // namespace

int main() {
    std::setvbuf(stdout, nullptr, _IONBF, 0);  // 实时输出，便于定位
    std::cout << "mtPool delayed-task demo\n";
    DemoParallelDelayed();
    DemoSequenceTokenSerial();
    DemoCancel();
    DemoSubmitFuture();
    DemoNamedToken();
    DemoCancelBreaksPromise();
    DemoSelfWaitDetected();
    DemoCancelWakesWaitUntilIdle();
    std::cout << "\n==== " << (g_failures == 0 ? "ALL PASSED" : "FAILED") << " (" << g_failures
              << " failure(s)) ====\n";
    return g_failures == 0 ? 0 : 1;
}
