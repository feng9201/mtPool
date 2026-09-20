#include <atomic>
#include <chrono>
#include <cstdint>
#include <iostream>
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

}  // namespace

int main() {
    std::cout << "mtPool delayed-task demo\n";
    DemoParallelDelayed();
    DemoSequenceTokenSerial();
    DemoCancel();
    DemoSubmitFuture();
    DemoNamedToken();
    std::cout << "\n==== " << (g_failures == 0 ? "ALL PASSED" : "FAILED") << " (" << g_failures
              << " failure(s)) ====\n";
    return g_failures == 0 ? 0 : 1;
}
