#pragma once

#include "mtPool/sequence_token.h"

#include <cstdint>
#include <unordered_set>

namespace mtPool {

// Tracks which sequence tokens currently have a running task.
// Unsequenced tokens (invalid) never occupy a slot.
class SequenceTracker {
public:
    bool IsRunnable(const SequenceToken& token) const;
    void Acquire(const SequenceToken& token);
    void Release(const SequenceToken& token);
    bool HasRunning() const;

private:
    std::unordered_set<std::uint64_t> running_;
};

}  // namespace mtPool
