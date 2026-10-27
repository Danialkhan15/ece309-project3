#include "tools/tool_registry.h"

#include <cstdint>
#include <utility>

ToolRegistry::ToolRegistry() : buckets_(16) {}
ToolRegistry::~ToolRegistry() = default;

std::size_t ToolRegistry::hash(std::string_view key) noexcept {
    // FNV-1a: unsigned arithmetic wraps, and every byte affects the hash.
    std::uint64_t value = 14695981039346656037ULL;
    for (unsigned char byte : key) {
        value ^= byte;
        value *= 1099511628211ULL;
    }
    return static_cast<std::size_t>(value);
}

void ToolRegistry::rehash(std::size_t new_bucket_count) {
    std::vector<std::list<Entry>> next(new_bucket_count);
    for (auto& bucket : buckets_) {
        while (!bucket.empty()) {
            const auto index = hash(bucket.front().name) % new_bucket_count;
            // Splice transfers an existing node without moving the Tool or
            // allocating another node. All allocations precede this loop.
            next[index].splice(next[index].end(), bucket, bucket.begin());
        }
    }
    buckets_.swap(next);
}

bool ToolRegistry::add(std::unique_ptr<Tool> tool) {
    if (!tool) return false;
    std::string name = tool->name();
    if (find(name) != nullptr) return false;

    // Separate chaining allows a threshold of 1.0. Grow before inserting.
    if (size_ == buckets_.size()) rehash(buckets_.size() * 2);
    const auto index = hash(name) % buckets_.size();
    buckets_[index].push_back({std::move(name), std::move(tool)});
    ++size_;
    return true;
}

Tool* ToolRegistry::find(std::string_view name) const {
    const auto index = hash(name) % buckets_.size();
    for (const auto& entry : buckets_[index]) {
        if (entry.name == name) return entry.tool.get();
    }
    return nullptr;
}

std::size_t ToolRegistry::size() const noexcept { return size_; }

void ToolRegistry::for_each(const std::function<void(const Tool&)>& fn) const {
    for (const auto& bucket : buckets_) {
        for (const auto& entry : bucket) fn(*entry.tool);
    }
}

std::size_t ToolRegistry::bucket_count() const noexcept { return buckets_.size(); }

double ToolRegistry::load_factor() const noexcept {
    return static_cast<double>(size_) / static_cast<double>(buckets_.size());
}

std::size_t ToolRegistry::max_probe_length() const {
    std::size_t longest = 0;
    for (const auto& bucket : buckets_) {
        if (bucket.size() > longest) longest = bucket.size();
    }
    return longest;
}
