import java.util.LinkedHashMap;
import java.util.Map;

public final class LruCache {

    private final Map<Integer, Integer> entries;

    public LruCache(int capacity) {
        if (capacity <= 0) {
            throw new IllegalArgumentException("capacity must be positive");
        }
        this.entries = new LinkedHashMap<>(16, 0.75f, true) {
            @Override
            protected boolean removeEldestEntry(Map.Entry<Integer, Integer> eldest) {
                return size() > capacity;
            }
        };
    }

    public int get(int key) {
        Integer value = entries.get(key);
        return value == null ? -1 : value;
    }

    public void put(int key, int value) {
        entries.put(key, value);
    }
}
