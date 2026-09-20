#pragma once

#include "mtPool/export.h"

#include <cstdint>
#include <string>

namespace mtPool {

// Opaque sequencing id, modeled after base::SequencedWorkerPool::SequenceToken.
// Invalid token (id == 0) means unsequenced: tasks may run in parallel.
class MTPOOL_API SequenceToken {
public:
    SequenceToken() = default;

    bool IsValid() const;
    std::uint64_t id() const;
    bool Equals(const SequenceToken& other) const;

    // Process-wide unique token.
    static SequenceToken Create();

    // Same name always maps to the same token.
    static SequenceToken Named(const std::string& name);

private:
    explicit SequenceToken(std::uint64_t id);
    std::uint64_t id_ = 0;
};

}  // namespace mtPool
