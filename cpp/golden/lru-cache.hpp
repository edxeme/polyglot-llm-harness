#pragma once

#include <cstddef>
#include <list>
#include <stdexcept>
#include <unordered_map>
#include <utility>

// O(1) LRU: list iterators stay valid across splice, so the index never
// needs rebuilding.
class LruCache {
public:
    explicit LruCache(std::size_t capacity) : capacity_(capacity) {
        if (capacity == 0) throw std::invalid_argument("capacity must be positive");
    }

    long long get(long long key) {
        auto it = index_.find(key);
        if (it == index_.end()) return -1;
        items_.splice(items_.begin(), items_, it->second);
        return it->second->second;
    }

    void put(long long key, long long value) {
        auto it = index_.find(key);
        if (it != index_.end()) {
            it->second->second = value;
            items_.splice(items_.begin(), items_, it->second);
            return;
        }
        if (items_.size() == capacity_) {
            index_.erase(items_.back().first);
            items_.pop_back();
        }
        items_.push_front({key, value});
        index_[key] = items_.begin();
    }

private:
    using Entry = std::pair<long long, long long>;
    std::size_t capacity_;
    std::list<Entry> items_;  // front = most recently used
    std::unordered_map<long long, std::list<Entry>::iterator> index_;
};
