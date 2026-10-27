import java.util.HashMap;
import java.util.Map;

// Candidate with a classic LRU bug: get() never refreshes recency and the
// eviction order is whatever HashMap iterates first.
public final class LruCache {

    private final Map<Integer, Integer> entries = new HashMap<>();
    private final int capacity;

    public LruCache(int capacity) {
        if (capacity <= 0) {
            throw new IllegalArgumentException("capacity must be positive");
        }
        this.capacity = capacity;
    }

    public int get(int key) {
        Integer value = entries.get(key);
        return value == null ? -1 : value;
    }

    public void put(int key, int value) {
        if (entries.size() >= capacity && !entries.containsKey(key)) {
            entries.remove(entries.keySet().iterator().next());
        }
        entries.put(key, value);
    }
}
