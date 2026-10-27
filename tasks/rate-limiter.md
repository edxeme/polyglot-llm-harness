# Task: rate-limiter

Implement a sliding-window rate limiter. A limiter allows at most `limit`
requests in any window of `window` milliseconds. Time is passed in by the
caller, so the implementation never reads the clock.

## Contract

| language | signature |
| :--- | :--- |
| Java | `class RateLimiter { RateLimiter(int limit, long windowMs); boolean allow(long nowMs); }` |
| Rust | `RateLimiter::new(limit: usize, window_ms: u64)`, `allow(&mut self, now_ms: u64) -> bool` |
| C++ | `class RateLimiter { RateLimiter(size_t limit, long long windowMs); bool allow(long long nowMs); }` |

## Requirements

- Timestamps are non-decreasing across calls. A decreasing timestamp is a
  caller error and must not corrupt state.
- A rejected request is not recorded.
- Memory is bounded by `limit`, not by the number of calls.

## Examples

- limit 2, window 1000: `allow(0)`, `allow(10)` → true, true; `allow(20)` → false;
  `allow(1000)` → true (the request at `0` has left the window)
