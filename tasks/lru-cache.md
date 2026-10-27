# Task: lru-cache

Implement a fixed-capacity least-recently-used cache keyed by 64-bit integers.
Both `get` and `put` count as "use". When a `put` would exceed capacity, evict
the least recently used key first. A `get` on a missing key returns `-1`.

## Contract

| language | signature |
| :--- | :--- |
| Java | `class LruCache { LruCache(int capacity); int get(int key); void put(int key, int value); }` |
| Rust | `LruCache::new(capacity: usize)`, `get(&mut self, key: i64) -> i64`, `put(&mut self, key: i64, value: i64)` |
| C++ | `class LruCache { explicit LruCache(size_t capacity); long long get(long long key); void put(long long key, long long value); }` |

## Requirements

- Constructor rejects non-positive capacity.
- `get` and `put` must refresh recency.
- `put` on an existing key updates the value and recency, without eviction.
- Target complexity: `O(1)` per operation.

## Examples

- cap 2: `put(1,10), put(2,20), put(3,30)` → `get(1) == -1`, `get(3) == 30`
- cap 2: `put(1,10), put(2,20), get(1), put(3,30)` → `get(2) == -1` (the `get`
  refreshed key 1, so key 2 is evicted)
