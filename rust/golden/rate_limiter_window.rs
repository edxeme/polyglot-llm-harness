use std::collections::VecDeque;

/// Sliding-window limiter: at most `limit` allowed requests in any window of
/// `window_ms`. Timestamps are supplied by the caller and must be
/// non-decreasing. Rejected requests are not recorded, so memory stays
/// bounded by the limit.
pub struct RateLimiter {
    limit: usize,
    window_ms: u64,
    hits: VecDeque<u64>,
}

impl RateLimiter {
    pub fn new(limit: usize, window_ms: u64) -> Self {
        assert!(limit > 0 && window_ms > 0, "limit and window must be positive");
        RateLimiter {
            limit,
            window_ms,
            hits: VecDeque::new(),
        }
    }

    pub fn allow(&mut self, now_ms: u64) -> bool {
        while self
            .hits
            .front()
            .is_some_and(|&first| first.saturating_add(self.window_ms) <= now_ms)
        {
            self.hits.pop_front();
        }
        if self.hits.len() < self.limit {
            self.hits.push_back(now_ms);
            true
        } else {
            false
        }
    }
}
