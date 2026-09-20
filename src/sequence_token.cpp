#include "mtPool/sequence_token.h"

#include <atomic>
#include <mutex>
#include <string>
#include <unordered_map>

namespace mtPool {
namespace {

std::atomic<std::uint64_t> g_next_token_id{1};
std::mutex g_named_mutex;
std::unordered_map<std::string, std::uint64_t> g_named_tokens;

}  // namespace

SequenceToken::SequenceToken(std::uint64_t id) : id_(id) {}

bool SequenceToken::IsValid() const {
    return id_ != 0;
}

std::uint64_t SequenceToken::id() const {
    return id_;
}

bool SequenceToken::Equals(const SequenceToken& other) const {
    return id_ == other.id_;
}

SequenceToken SequenceToken::Create() {
    return SequenceToken(g_next_token_id.fetch_add(1));
}

SequenceToken SequenceToken::Named(const std::string& name) {
    std::lock_guard<std::mutex> lock(g_named_mutex);
    auto it = g_named_tokens.find(name);
    if (it != g_named_tokens.end()) {
        return SequenceToken(it->second);
    }
    const auto id = g_next_token_id.fetch_add(1);
    g_named_tokens.emplace(name, id);
    return SequenceToken(id);
}

}  // namespace mtPool
