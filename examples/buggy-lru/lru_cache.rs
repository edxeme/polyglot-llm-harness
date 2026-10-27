use std::collections::HashMap;

// Candidate with a classic LRU bug: get() never refreshes recency and the
// eviction order is whatever HashMap yields first.
pub struct LruCache {
    capacity: usize,
    entries: HashMap<i64, i64>,
}

impl LruCache {
    pub fn new(capacity: usize) -> Self {
        assert!(capacity > 0, "capacity must be positive");
        LruCache { capacity, entries: HashMap::new() }
    }

    pub fn get(&self, key: i64) -> i64 {
        *self.entries.get(&key).unwrap_or(&-1)
    }

    pub fn put(&mut self, key: i64, value: i64) {
        if self.entries.len() == self.capacity && !self.entries.contains_key(&key) {
            let first = *self.entries.keys().next().unwrap();
            self.entries.remove(&first);
        }
        self.entries.insert(key, value);
    }
}
