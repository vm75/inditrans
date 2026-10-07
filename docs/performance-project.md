# C++23 engine performance project

The original performance milestones are complete. The current implementation and
reproduction procedure are described in [Architecture](../ARCHITECTURE.md) and
the [performance guide](performance.md). The original experiment chronology is
preserved in Git history. The subsequent [local reassessment](performance-reassessment.md)
compares against `perf-00-baseline`; it has not been submitted to hosted CI.

## Milestone status

| Milestone | Status | Result |
|---|---|---|
| P0 — measurement and baseline tooling | Complete | Repeated throughput, latency, cold-start, allocation, and artifact measurements are available. |
| P1 — output ownership | Complete | C buffer ownership is explicit and sanitizer-covered. |
| P2 — cold-start measurement | Complete | Fresh-process first-call measurements cover four representative paths. |
| P3 — remove Tamil writer hash lookup | Complete | Static comparisons preserve behavior. |
| P4 — stack reader/writer state | Complete | Per-call ownership avoids heap allocation for these objects. |
| P5 — immutable generated metadata | Complete | Readable typed UTF-8 declarations transform at compile time to immutable lookup tables. |
| P6 — packed static trie | Complete | Reachable states, direct terminal fields, compressed paths, and dispatch pages are immutable. |
| P7 — name resolution and streaming/sink pipeline | Complete | Names resolve statically; input streams through bounded lookahead and output uses sinks. |
| P8 — final platform and release acceptance | Complete | All hosted acceptance jobs passed on `e302ae3`; the clean-tree five-run baseline and host toolchain are recorded in the [performance guide](performance.md), and the [platform matrix](platform-support.md) records platform evidence. |

The reassessment selects one of three writer-type specializations per call,
with a shared matcher and two output sinks. It generates no per-script-pair code.
Reader expansions use a fixed stack window and a plain policy function pointer.

## Accepted trade-offs

- Standalone Wasm is 89,336 bytes (87.24 KiB), using Emscripten 6.0.10 with
  `-Oz -flto`. The binary stores direct
  immutable lookup data and avoids runtime metadata construction. Smaller
  static encodings were rejected after repeatable hot-path regressions.
- Compile-time transformation increases clean compile time and compiler
  memory. This is accepted to keep semantic declarations readable while
  emitting only runtime data.

Keep layout and artifact changes evidence-driven. Compare repeated native and
Wasm timings, output hashes, and allocation metrics before retaining a change.
