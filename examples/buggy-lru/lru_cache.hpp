#pragma once

#include <cstddef>
#include <stdexcept>
#include <unordered_map>

// Candidate with a classic LRU bug: get() never refreshes recency and the
// eviction order is whatever the unordered_map bucket order yields.
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
            entries_.erase(entries_.begin());
        }
        entries_[key] = value;
    }

private:
    std::size_t capacity_;
    std::unordered_map<long long, long long> entries_;
};
