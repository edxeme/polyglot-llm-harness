#pragma once

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <unordered_map>

// Candidate with a classic LRU bug: get() never refreshes recency and the
// eviction victim is the lowest key (deterministic — but never the
// least-recently-used one).
class LruCache {
public:
    explicit LruCache(std::size_t capacity) : capacity_(capacity) {
        if (capacity == 0) throw std::invalid_argument("capacity must be positive");
    }

    long long get(long long key) {
        auto it = entries_.find(key);
        return it == entries_.end() ? -1 : it->second;
    }

    void put(long long key, long long value) {
        if (entries_.size() == capacity_ && entries_.find(key) == entries_.end()) {
            auto victim = std::min_element(entries_.begin(), entries_.end(),
                                           [](const auto& a, const auto& b) { return a.first < b.first; });
            entries_.erase(victim);
        }
        entries_[key] = value;
    }

private:
    std::size_t capacity_;
    std::unordered_map<long long, long long> entries_;
};
