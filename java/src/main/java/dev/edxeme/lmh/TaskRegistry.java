package dev.edxeme.lmh;

import java.util.List;

final class TaskRegistry {

    private TaskRegistry() {}

    static final List<TaskSpec> ALL = List.of(
            new TaskSpec("merge-intervals", "MergeIntervals.java", "java/verifier/cases/merge-intervals.cases"),
            new TaskSpec("lru-cache", "LruCache.java", "java/verifier/cases/lru-cache.cases"),
            new TaskSpec("rate-limiter", "RateLimiter.java", "java/verifier/cases/rate-limiter.cases"));

    static TaskSpec find(String id) {
        for (TaskSpec t : ALL) {
            if (t.id().equals(id)) {
                return t;
            }
        }
        return null;
    }
}
