pub fn merge(intervals: &[(i64, i64)]) -> Vec<(i64, i64)> {
    for (start, end) in intervals {
        assert!(start <= end, "invalid interval: [{start},{end}]");
    }
    let mut sorted = intervals.to_vec();
    sorted.sort_unstable();
    let mut merged: Vec<(i64, i64)> = Vec::with_capacity(sorted.len());
    for (start, end) in sorted {
        match merged.last_mut() {
            Some(last) if start <= last.1 => {
                if end > last.1 {
                    last.1 = end;
                }
            }
            _ => merged.push((start, end)),
        }
    }
    merged
}
