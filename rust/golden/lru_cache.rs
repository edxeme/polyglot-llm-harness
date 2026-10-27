use std::collections::HashMap;

/// O(1) LRU via a slot-backed doubly linked list: no Rc/RefCell, no unsafe.
struct Node {
    key: i64,
    value: i64,
    prev: Option<usize>,
    next: Option<usize>,
}

pub struct LruCache {
    capacity: usize,
    nodes: Vec<Node>,
    free: Vec<usize>,
    map: HashMap<i64, usize>,
    head: Option<usize>,
    tail: Option<usize>,
}

impl LruCache {
    pub fn new(capacity: usize) -> Self {
        assert!(capacity > 0, "capacity must be positive");
        LruCache {
            capacity,
            nodes: Vec::new(),
            free: Vec::new(),
            map: HashMap::new(),
            head: None,
            tail: None,
        }
    }

    fn detach(&mut self, idx: usize) {
        let (prev, next) = (self.nodes[idx].prev, self.nodes[idx].next);
        match prev {
            Some(p) => self.nodes[p].next = next,
            None => self.head = next,
        }
        match next {
            Some(n) => self.nodes[n].prev = prev,
            None => self.tail = prev,
        }
        self.nodes[idx].prev = None;
        self.nodes[idx].next = None;
    }

    fn push_front(&mut self, idx: usize) {
        self.nodes[idx].prev = None;
        self.nodes[idx].next = self.head;
        if let Some(h) = self.head {
            self.nodes[h].prev = Some(idx);
        }
        self.head = Some(idx);
        if self.tail.is_none() {
            self.tail = Some(idx);
        }
    }

    fn move_to_front(&mut self, idx: usize) {
        if self.head != Some(idx) {
            self.detach(idx);
            self.push_front(idx);
        }
    }

    pub fn get(&mut self, key: i64) -> i64 {
        match self.map.get(&key).copied() {
            Some(idx) => {
                let value = self.nodes[idx].value;
                self.move_to_front(idx);
                value
            }
            None => -1,
        }
    }

    pub fn put(&mut self, key: i64, value: i64) {
        if let Some(idx) = self.map.get(&key).copied() {
            self.nodes[idx].value = value;
            self.move_to_front(idx);
            return;
        }
        if self.map.len() == self.capacity {
            let tail = self.tail.expect("full cache must have a tail");
            let evicted = self.nodes[tail].key;
            self.map.remove(&evicted);
            self.detach(tail);
            self.free.push(tail);
        }
        let idx = self.free.pop().unwrap_or_else(|| {
            self.nodes.push(Node { key, value, prev: None, next: None });
            self.nodes.len() - 1
        });
        self.nodes[idx].key = key;
        self.nodes[idx].value = value;
        self.map.insert(key, idx);
        self.push_front(idx);
    }
}
