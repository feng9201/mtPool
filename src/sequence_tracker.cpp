#include "mtPool/sequence_tracker.h"

namespace mtPool {

bool SequenceTracker::IsRunnable(const SequenceToken& token) const {
    if (!token.IsValid()) {
        return true;
    }
    return running_.find(token.id()) == running_.end();
}

void SequenceTracker::Acquire(const SequenceToken& token) {
    if (token.IsValid()) {
        running_.insert(token.id());
    }
}

void SequenceTracker::Release(const SequenceToken& token) {
    if (token.IsValid()) {
        running_.erase(token.id());
    }
}

bool SequenceTracker::HasRunning() const {
    return !running_.empty();
}

}  // namespace mtPool
