#pragma once

#include <cstddef>
#include <deque>
#include <stdexcept>

// Sliding-window limiter: at most limit allowed requests in any window of
// windowMs. Timestamps are supplied by the caller and must be
// non-decreasing. Rejected requests are not recorded, so memory stays
// bounded by the limit.
class RateLimiter {
public:
    RateLimiter(std::size_t limit, long long window_ms) : limit_(limit), window_ms_(window_ms) {
        if (limit == 0 || window_ms <= 0) throw std::invalid_argument("limit and window must be positive");
    }

    bool allow(long long now_ms) {
        while (!hits_.empty() && hits_.front() + window_ms_ <= now_ms) {
            hits_.pop_front();
        }
        if (hits_.size() < limit_) {
            hits_.push_back(now_ms);
            return true;
        }
        return false;
    }

private:
    std::size_t limit_;
    long long window_ms_;
    std::deque<long long> hits_;
};
