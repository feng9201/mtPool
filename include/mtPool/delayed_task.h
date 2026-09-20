#pragma once

#include "mtPool/clock.h"
#include "mtPool/cancel_state.h"
#include "mtPool/sequence_token.h"
#include "mtPool/task.h"

#include <cstdint>

namespace mtPool {

struct DelayedTask {
    Task task;
    TimePoint run_at;
    std::uint64_t sequence_num = 0;
    SequenceToken token;
    CancelStatePtr cancel;
};

struct ScheduleKey {
    TimePoint run_at;
    std::uint64_t sequence_num = 0;

    bool operator<(const ScheduleKey& other) const {
        if (run_at != other.run_at) {
            return run_at < other.run_at;
        }
        return sequence_num < other.sequence_num;
    }
};

inline ScheduleKey MakeScheduleKey(const DelayedTask& task) {
    return ScheduleKey{task.run_at, task.sequence_num};
}

}  // namespace mtPool
