# Implementation notes

Observations from building the same protocol three times, plus the numbers I
measured along the way. Machine: M-series MacBook Pro, macOS, JDK 26, rustc
1.99, Apple clang 21.

## Line counts (source + goldens + case tables)

| impl | lines | files |
| :--- | ---: | ---: |
| java | 834 | 14 |
| rust | 922 | 12 |
| cpp | 1003 | 13 |

C++ pays a constant tax for splitting header/impl and for the generated-code
plumbing (`std::string` assembly everywhere). Rust is denser than expected —
the JSON parser is compact and the golden solutions are terse without being
clever. Java would shrink further with records, but the harness predates
needing them.

## Cold verifier run (compose + build + grade 8 cases, golden)

| impl | wall time | build step |
| :--- | ---: | :--- |
| java | ~1.0s | `javac` (JVM startup dominates) |
| rust | ~1.5s | `rustc -O` single file |
| cpp | ~2.0s | `c++ -std=c++17 -O2` |

The case themselves run in microseconds everywhere. The build step is the
budget, which is exactly why a real harness batches cases into one verifier
program instead of one process per case.

## Things each language made easy / annoying

**Java**

- Easy: `ProcessBuilder` with redirected streams is the cleanest child-process
  handling of the three; records + switch expressions keep the report layer
  tiny.
- Annoying: writing the generated driver as a text block fights escaping —
  the driver lives in a plain template file instead (`driver.java.template`).
- The `LinkedHashMap(accessOrder=true)` golden is the only golden that leans
  on a standard-library trick; the others are hand-rolled structures.

**Rust**

- Easy: `catch_unwind` + a panic hook gives per-case fault isolation for
  free — candidates can panic without taking down the run. No equivalent is
  needed in the exception languages.
- Annoying: single-file `rustc` compilation is the right tool here (cargo
  would mean generating a whole crate per run), but it forces the
  `mod candidate;` file layout, which is why candidates are always copied
  under a canonical name.
- The LRU golden is a slot-backed doubly linked list: no `Rc<RefCell<…>>`,
  no `unsafe`, still O(1).

**C++**

- Easy: once the mini-JSON layer exists, everything else is ordinary C++17.
  `std::list::splice` makes the LRU golden short and iterator-stable.
- Annoying: `popen` means shell quoting and `WEXITSTATUS` bookkeeping; the
  compile step dominates and there is no cheap way to precompile per-case.
  Value semantics do earn their keep — no allocation ceremony anywhere.

## Report parity in practice

The three parsers read the same JSONL contract, so parity bugs show up as
`diff` failures in CI (the workflow normalizes away `impl`, `source` and all
timings). The one parity trap was number formatting: `%.3f` everywhere, and
integer counts parsed through `double` in every implementation to keep the
schema uniform.

## Known limitations (v1)

- Process isolation only — no rlimits/seccomp/container sandboxing.
- Sequential case execution; the build step dwarfs case cost anyway.
- Single-file candidates; a real harness would accept file bundles with
  language-specific project layouts.
- `popen` (C++) and process timeout (60s) are the weakest links; a serious
  run would add wall-clock limits per case and memory caps.
