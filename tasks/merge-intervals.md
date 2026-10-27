# Task: merge-intervals

Given a list of closed integer intervals `[start, end]`, return the minimal set
of disjoint intervals that covers the same points. Overlapping and touching
intervals (`[1,3]` and `[3,5]`) are merged. The output is sorted by `start`.

## Contract

| language | signature |
| :--- | :--- |
| Java | `static int[][] merge(int[][] intervals)` |
| Rust | `pub fn merge(intervals: &[(i64, i64)]) -> Vec<(i64, i64)>` |
| C++ | `std::vector<std::pair<long long,long long>> merge(std::vector<std::pair<long long,long long>> intervals)` |

## Requirements

- The input is not sorted and may be empty.
- Do not mutate the caller's input.
- An interval with `start > end` is invalid: throw or panic.

## Examples

- `[[1,3],[2,6],[8,10],[15,18]]` → `[[1,6],[8,10],[15,18]]`
- `[[1,4],[4,5]]` → `[[1,5]]`
- `[]` → `[]`
