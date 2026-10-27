#pragma once

#include <algorithm>
#include <stdexcept>
#include <utility>
#include <vector>

using Interval = std::pair<long long, long long>;

inline std::vector<Interval> merge(std::vector<Interval> intervals) {
    for (const auto& [start, end] : intervals) {
        if (start > end) throw std::invalid_argument("invalid interval: [start,end]");
    }
    std::vector<Interval> sorted = intervals;
    std::sort(sorted.begin(), sorted.end());
    std::vector<Interval> merged;
    merged.reserve(sorted.size());
    for (const auto& [start, end] : sorted) {
        if (!merged.empty() && start <= merged.back().second) {
            merged.back().second = std::max(merged.back().second, end);
        } else {
            merged.push_back({start, end});
        }
    }
    return merged;
}
