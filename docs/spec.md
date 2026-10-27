# lmh — verifier protocol

`lmh` grades candidate solutions to small coding tasks. One protocol, three
independent implementations (Java, Rust, C++) that must behave identically:
same CLI, same case tables, same report.

## Motivation

Grading a candidate solution is a build-and-run problem under an untrusted-code
constraint. Keeping the protocol small and implementing it three times makes
the trade-offs (process handling, string handling, temp-file hygiene) visible
instead of hiding them behind a framework.

## Model

- **Task**: a coding problem with a fixed API contract and a table of named
  cases. Statements live in `tasks/`.
- **Candidate**: a single source file implementing the contract. Either the
  bundled golden solution or any file passed with `--candidate`.
- **Verifier**: a program the harness generates from the candidate plus a
  driver with the case table. It prints one JSON line per case and a summary.
- **Harness**: composes the verifier in a throwaway directory, builds it with
  the platform toolchain, runs it, and emits a report.

## Isolation rule

A candidate is untrusted input. The harness never loads candidate code into
its own process. It copies the candidate into a temp directory, generates the
driver, compiles a standalone verifier binary and runs it as a child process.
The temp directory is disposable.

## Verifier contract

One JSON object per case on stdout:

```json
{"case":"hit_returns_value","status":"pass","ms":0.031,"detail":""}
```

followed by exactly one summary line:

```json
{"summary":{"total":8,"passed":8,"failed":0}}
```

- `status` is `pass` or `fail`.
- `ms` is wall time for the case, in fractional milliseconds.
- `detail` is a short escaped message on failure, empty on pass.
- Exit code `0` iff `failed == 0`, `1` otherwise.

The harness parses stdout as JSONL. Stderr is forwarded to the run log and
never parsed.

## Report schema

`lmh.report/v1`, printed by the harness to stdout:

```json
{
  "schema": "lmh.report/v1",
  "task": "lru-cache",
  "impl": "rust",
  "source": "golden",
  "duration_ms": 9.7,
  "summary": {"total": 8, "passed": 8, "failed": 0},
  "score": 1.0,
  "cases": [
    {"name": "hit_returns_value", "status": "pass", "ms": 0.031, "detail": ""}
  ]
}
```

- `source` is `golden` or the candidate path.
- `score` is `passed / total`, or `0.0` when the verifier could not be built
  or crashed.
- `duration_ms` covers compose, build and run.

On build failure the summary is `{"total": 0, "passed": 0, "failed": 0}` and
compiler diagnostics go to stderr.

## CLI

Identical across implementations:

```
lmh tasks                                   # list registered tasks
lmh run <task> [--candidate <file>] [--json]
```

Without `--candidate` the bundled golden solution is graded. `--json` switches
stdout from the human table to the report object.

## Exit codes

| code | meaning |
| :--- | :--- |
| 0 | all cases passed |
| 1 | verifier ran, some case failed |
| 2 | candidate or verifier failed to build |
| 3 | usage error (unknown task, unreadable candidate) |

## Scope

In scope: deterministic, dependency-free grading of single-file candidates in
a subprocess, with equivalent reports across implementations.

Out of scope for v1: sandboxing beyond process isolation (rlimits, namespaces),
parallel case execution, network stubbing, model-client integrations.
