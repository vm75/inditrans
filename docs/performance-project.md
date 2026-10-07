# C++23 engine performance project

The performance architecture is complete. The final implementation and
reproduction procedure are described in [Architecture](../ARCHITECTURE.md) and
the [performance guide](performance.md). The original experiment chronology is
preserved in Git history.

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

The former script-type specialization proposal was not adopted. The selected
shared matcher and sink pipeline provide the final architecture without
per-script-pair generated code.

## Accepted trade-offs

- Standalone Wasm is approximately 89.73 KiB. The binary stores direct
  immutable lookup data and avoids runtime metadata construction. Smaller
  static encodings were rejected after repeatable hot-path regressions.
- Compile-time transformation increases clean compile time and compiler
  memory. This is accepted to keep semantic declarations readable while
  emitting only runtime data.

Neither trade-off is an open optimization target. Revisit the runtime layout
only to fix a demonstrated correctness/portability issue or with complete
benchmark evidence.
