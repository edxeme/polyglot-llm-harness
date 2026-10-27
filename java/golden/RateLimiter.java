import java.util.ArrayDeque;
import java.util.Deque;

/**
 * Sliding-window limiter: at most limit allowed requests in any window of
 * windowMs. Timestamps are supplied by the caller and must be
 * non-decreasing. Rejected requests are not recorded, so memory stays
 * bounded by the limit.
 */
public final class RateLimiter {

    private final long limit;
    private final long windowMs;
    private final Deque<Long> hits = new ArrayDeque<>();

    public RateLimiter(int limit, long windowMs) {
        if (limit <= 0 || windowMs <= 0) {
            throw new IllegalArgumentException("limit and window must be positive");
        }
        this.limit = limit;
        this.windowMs = windowMs;
    }

    public boolean allow(long nowMs) {
        while (!hits.isEmpty() && hits.peekFirst() + windowMs <= nowMs) {
            hits.pollFirst();
        }
        if (hits.size() < limit) {
            hits.addLast(nowMs);
            return true;
        }
        return false;
    }
}
