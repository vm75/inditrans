# Performance reassessment

Local capture: 2026-10-07. Reference: `perf-00-baseline` (locally
`0898c5d3cca74abd81e6846761d7003eca8ddebe`). The branch started at `f30fec7`.
Tagged engine sources were unchanged; both engines used the current harness.
All changes and measurements remain local. Hosted CI has not run this revision.

## Selected implementation

- Immutable lookup tables still come from readable typed declarations and C++23
  compile-time builders. The declarations introduce no runtime parsing or construction.
- A compact pointer/word entry and fixed 16-entry reader window replace vector
  allocations. Refills reuse consumed storage without shifting or ring arithmetic.
- A plain function pointer selects reader policy; one dispatch selects writer type.
  Token grouping, source precedence, and output rules are unchanged.
- Marker filtering appends retained runs together. A compile-time check permits
  the ASCII shortcut while retaining the original non-ASCII substring predicate.
- Fresh Roman-to-non-Latin results reserve three times the input length as a
  growth hint. Append calls retain their previous reservation behavior.
- Writer text shares substrings and suffixes. Checked glyph literals and direct
  offset/length lookup remain. Release Wasm uses `-Oz -flto`.

Ring/chunk readers, buffered output chunks, and full reader specialization were
not retained after mixed or slower timings. Removing the forced trie-transition
`noinline` lets each compiler choose its own inlining policy.

## Measurement conditions

Same host and CPU 0 for each comparison: Intel Core Ultra 9 185H, WSL2 Linux.
Native flags: `-std=c++23 -O3 -DNDEBUG`.

| Configuration | Compiler and runtime | Repeats |
|---|---|---|
| Native 18 | Clang 18.1.3, GCC 14 headers/runtime, Ubuntu glibc 2.39 | Three full throughput sweeps; five short-call sweeps |
| Native 23 | Clang 23.1.1, libstdc++ 16, glibc 2.42 | Three full throughput sweeps; five short-call sweeps |
| Wasm | Emscripten 6.0.10, Node 25.8.1 | Three full sweeps, with and without LTO |
| Native cold calls | Clang 23 | 501 fresh processes per path |

Native throughput covers ten workloads at short, approximately 4 KiB, and
1 MiB input sizes. Wasm covers the same workloads at short, 4 KiB, and 64 KiB:
the tag's eager reader exceeds its original standalone memory at 1 MiB.
Native timings use the returned-string API; Wasm timings use the C ABI and
include result release, excluding decoding and hashing. Compare revisions
within each configuration, rather than comparing native and Wasm absolute times.

## Results against the tag

All 30 throughput cells improve on both native compilers. Changes below are
in elapsed time; negative values mean faster calls. Each cell is the arithmetic
mean of the three per-run medians. Every output size and FNV-1a hash agrees.

| Workload | Native 18, 1 MiB | Native 23, 1 MiB | Wasm with LTO, 64 KiB |
|---|---:|---:|---:|
| Indic → Indic | −10.2% | −13.7% | −52.3% |
| Tamil output | −11.5% | −10.4% | −47.5% |
| Latin input | −17.3% | −14.8% | −54.7% |
| Virtual Indic → Latin | −11.6% | −16.9% | −62.6% |
| Virtual Indic → Indic | −9.2% | −9.7% | −57.3% |
| Virtual Indic → Tamil | −13.8% | −15.7% | −53.9% |
| Latin output | −14.6% | −21.9% | −60.1% |
| Expansion-heavy | −16.5% | −12.9% | −57.8% |
| Protected spans | −42.8% | −42.7% | −81.5% |
| Mixed protected spans | −21.6% | −26.2% | −71.5% |

Across all sizes, native time reductions range from 3.9–65.8% on Clang 18 and
4.9–66.0% on Clang 23. Wasm reductions range from 47.5–81.5%.

Short-call p50 reductions on Clang 23 range from 18.8–57.4%. Clang 18's initial
five-run capture had a noisy first case: Indic → Indic ranged from 636–1,405 ns,
and its arithmetic mean was 12.7% above the tag. A separate five-run check warmed
CPU 0 with throughput work before **each** engine process, using the unchanged
short-call harness. All p50 values then improved, by 17.1–54.8%; Indic → Indic
was 675 ns versus 871 ns. Both raw captures are retained. These are local
measurements, not a universal latency guarantee or a hosted CI result.

Cold p50 falls from 167–297 μs to 6.7–8.2 μs across the four paths, with matching
hashes. Process launch is excluded.

### Allocations

Linux/glibc probe, approximately 1 MiB input. Peak is requested live allocator
bytes, not RSS. The selected engine's cold and warm results are identical,
with zero retained bytes and zero untracked events.

| Workload | Tag cold events | Selected events | Tag cold peak B | Selected peak B |
|---|---:|---:|---:|---:|
| Indic → Indic | 2,762 | 1 | 26,359,260 | 1,048,669 |
| Latin input | 3,022 | 1 | 29,512,781 | 3,145,796 |
| Virtual Indic | 6,712 | 1 | 26,484,604 | 1,048,669 |
| Expansion-heavy | 2,763 | 2 | 27,405,773 | 3,145,757 |
| Protected spans | 2,762 | 1 | 26,357,560 | 1,048,601 |
| Mixed protected spans | 2,762 | 1 | 26,359,235 | 1,048,668 |

The tag's warm calls still make 9–11 allocation events. Reader and reusable
metadata allocation is now zero; these remaining events belong to result
storage. The Roman reservation hint can exceed actual output capacity for raw
or mixed text; it avoids repeated growth on expanding transliteration input.
The separate metadata lookup probe also reports zero malloc/calloc/realloc/free,
zero requested live/peak bytes, and checksum 1047.

### Artifact size

| Artifact | Branch start | Selected |
|---|---:|---:|
| Standalone Wasm | 91,883 B | 89,336 B |
| JavaScript single-file distribution | 123,155 B | 121,049 B |

Standalone Wasm is 2.8% smaller than the branch start, and LTO improves execution
time in the measured matrix. The tag rebuilt with the earlier release flags is
58,479 B: the selected binary remains 52.8% larger because it stores immutable
lookup tables instead of constructing them at runtime. Binary size is a
remaining trade-off; smaller encodings require equivalent runtime evidence.

## Validation and reproduction

Native tests, the 405,251 packed-data assertions, Flutter's 47 tests, Node's
47 tests, standalone Wasm smoke testing, and release validation pass. The
`dart analyze` reports no issues. The native suite also passes AddressSanitizer and UndefinedBehaviorSanitizer;
LeakSanitizer is unavailable in the sandbox. Output hashes agree throughout
native, Wasm, and cold-call comparisons. Windows execution was not measured.

See [the reproduction guide](performance.md) for tag-based native builds and
`node tool/wasm_bench.mjs flutter/assets/inditrans.wasm` for the standalone
matrix. Pin both engines to the same CPU and use the same toolchain.

Raw local files are ignored under `out/reassessment/`:

- `verified-clang{18,23}-runs.csv` and the per-engine aggregate CSVs;
- `latency-control-clang18-runs.csv` and aggregate captures;
- `wasm-selected-runs.csv`, `wasm-final.csv`;
- `baseline-allocs*.csv`, `final-allocs*.csv`, `tag-cold.csv`, `final-cold.csv`;
- build, test, sanitizer, and release-validation logs.

Earlier experimental captures remain alongside them. Do not compare timings
from different machines or accept size reductions without checking runtime
performance and output hashes.
