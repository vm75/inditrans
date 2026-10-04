# Native Performance Tuning Plan

Status: active (partially superseded by static-lookup-design)

This plan replaces the abandoned broad C++23 performance plans with a sequence of small, independently measurable changes. The project already requires C++23; the goal here is to reduce runtime allocation, startup work, and hot-path overhead without changing transliteration behavior.

## Working rules

1. One performance hypothesis per step commit on `dev`; do not combine unrelated optimizations.
2. Keep the active `dev` → `main` PR open while iterating so every pushed step receives full CI validation before the next step starts.
3. Preserve transliteration output. Existing benchmark output hashes and the full test suite must remain unchanged unless a change is explicitly intended to alter behavior.
4. Measure before and after every step. Do not combine several structural optimizations into one benchmark result.
5. Prefer deleting runtime work over replacing one runtime container with another.
6. Track raw Wasm size alongside runtime performance so speed improvements do not silently cause unacceptable binary growth.
7. Treat allocation-probe `peak heap` as peak requested live heap bytes, not process RSS.
8. Keep the public C ABI stable unless a step explicitly requires an API addition.

## Measurement workflow

Performance progression snapshots live under `out/perf/` and are local artifacts; they are not committed.

Use numbered names so report ordering is deterministic:

```text
00-baseline
01-buffer-ownership
02-cold-start-benchmark
03-tamil-map
04-stack-reader-writer
05-writer-map-view
...
```

Capture a snapshot:

```bash
make perf-snapshot PERF_NAME=00-baseline
make bench-cold
```

For latency claims, use repeated full short-call runs rather than relying on one snapshot timing:

```bash
make bench-short-repeat
# Optional Linux CPU pinning:
make bench-short-repeat PERF_CPU=2
```

`PERF_REPEATS` defaults to 5 and must be an odd number of at least 3.

After each accepted step:

```bash
make perf-snapshot PERF_NAME=03-tamil-map
make perf-report
```

`make perf-snapshot` reuses the existing `bench-save` format, captures cold p50/p95 across fresh processes, force-rebuilds the standalone Wasm artifact, and records its raw byte size. `make perf-report` discovers the numbered snapshots and prints a Markdown progression report.

The default summary uses:

- warm short-call p50: `indic-to-indic`
- allocation mode: `devanagari`
- cold allocation count: `malloc + calloc + realloc` with no warmup; includes lazy initialization
- warm allocation count: the same probe after one unmeasured warmup; isolates recurring work
- cold/warm peak heap: allocator-probe peak requested live bytes
- raw standalone Wasm size: `flutter/assets/inditrans.wasm`
- cold-start p50/p95 in fresh native processes for Devanagari→Telugu, ISO→Devanagari, Devanagari→Tamil, and Indic→ISO

Percentage/count deltas use `00-baseline` when that metric exists there. Metrics introduced after the original baseline (currently warm allocations and the three additional cold-start cases) use the earliest snapshot containing that metric as their reference. The report identifies that reference and also prints absolute values for auditability.

Override the representative cases when needed:

```bash
make perf-report PERF_LATENCY_CASE=latin-input PERF_ALLOC_CASE=latin
make perf-report PERF_COLD_CASE=cold-iso-to-devanagari
```

Existing snapshots that predate warm-allocation capture remain readable; warm fields display as `—` until the first snapshot containing warm data establishes the warm reference. The same rule applies to newly added cold-start cases. Snapshot metadata records the commit, date, platform, CPU, Clang version, Emscripten state/required version, and whether the working tree was clean.

The existing `bench-compare` remains the PR regression gate across all benchmark cases and output hashes. The progression report is complementary: it shows cumulative movement from the original tuning baseline.

## Required checks for every implementation step

- `make test`
- output hashes unchanged in the benchmark harness
- use `make bench-short-repeat` for any claimed short-call latency improvement
- `make bench-compare` against the PR base
- `BENCH_STRICT=1 make bench-compare` before merge when the change is expected to affect performance
- sanitizers/CI green
- capture a numbered performance snapshot for the cumulative report
- record unexpected Wasm growth in the PR description

For structural changes, also run the Flutter/Node/Wasm tests affected by the change.

## Tracking

| ID | Step | Scope | Primary expected signal | Status | Result / PR |
|---|---|---|---|---|---|
| 0A | Add cumulative performance reporting | `tool/perf_report.py`, Make targets, plan | reproducible tracking | ✅ done | this setup change |
| 0B | Capture clean tuning baseline | snapshot current merged implementation before optimization | baseline only | ✅ done | `00-baseline` at `717b766`; p50 1.473 µs, 2,762 allocs, 25.14 MiB peak requested heap, 58,479 B Wasm |
| 1 | Fix output-buffer ownership mismatch | make `realloc`/`free` ownership explicit; remove mismatched default deleter; re-enable sanitizer mismatch detection | correctness; sanitizer clean | ✅ done | `01-buffer-ownership` snapshot: p50 1.372 µs, 2,762 allocs, 25.14 MiB peak requested heap, 58,469 B Wasm; ASan/UBSan/LSan and allocator mismatch detection pass in PR #52 CI |
| 2 | Add cold-start benchmark | process-isolated first transliteration measurement | cold p50/p95 | ✅ done | 501 fresh-process samples; exact `717b766` baseline p50/p95 215.980/453.430 µs; `02-cold-start-benchmark` 241.869/458.841 µs; output hash stable |
| 3 | Remove `OutputWriter` Tamil traditional hash map | replace three-entry `unordered_map` with constexpr lookup/comparisons | warm p50; allocations | ✅ done | all output hashes match; four fewer allocations per call and Wasm is 974 B smaller; nine alternating short-call runs showed p50 improvement of 7–30% across cases |
| 4 | Stack-allocate input reader/output writer | remove per-call `make_unique` reader/writer allocations | allocations; short-call p50 | ✅ done | all output hashes match; two fewer allocations and 168 fewer requested bytes per call; Wasm is 145 B smaller; paired short-call p50 was mixed/near-flat |
| 5 | Make `ScriptWriterMap` non-owning | replace copied `vector<string_view>` maps with spans/ranges/views | initialization allocations; cold latency | ✅ done | 8 fewer allocation calls during first-call allocation measurement; paired cold p50/p95 were 530.908/689.869 µs at Step 4 and 527.071/687.175 µs here (effectively flat); cold and benchmark output hashes match; Wasm is 465 B smaller |
| 6A | Generate constexpr script metadata alongside legacy data | generator emits immutable metadata/name/alias/range tables while runtime remains unchanged | parity only | ✅ done (via static lookup) | |
| 6B | Replace runtime `ScriptData` parsing | switch lookup to generated constexpr metadata and remove runtime maps/vectors | cold latency; startup allocations | ✅ done (via static lookup) | |
| 7A | Implement flat immutable trie type | `char32_t` edges, 32-bit child/index offsets, sorted child ranges; unit-test independently | parity only | ✅ done (via static lookup) | |
| 7B | Generate flat tries alongside runtime tries | generator emits trie nodes/edges and tests compare longest-match behavior | parity only; Wasm size | ✅ done (via static lookup) | |
| 7C | Replace runtime character tries | remove `unordered_map`/`unique_ptr` trie construction and temporary UTF-32 insertion strings | cold latency; allocations; throughput | ✅ done (via static lookup) | |
| 8 | Resolve names once to `ScriptId` and remove reader/writer caches | constexpr name/alias resolution; downstream uses small IDs/references | lookup overhead; startup allocations | ✅ done (via static lookup) | |
| 9 | Make `InputReader` streaming | remove full-input `tokenUnits` vector; retain only required lookahead | peak heap; large-input allocation bytes | ✅ done | implemented on-demand pull and small lookahead buffer |
| 10 | Add allocation-free sink/output API | templated sink core plus adapters for `std::string`, caller buffer/FFI as appropriate | output allocation; FFI efficiency | ✅ done | implemented templated OutputWriter and StdStringSink adapter |
| 11 | Specialize conversion by script type | dispatch once to `Indic`/`Tamil`/`Latin` template specializations; use `if constexpr` internally | hot-path throughput; Wasm code size | ✅ done | implemented template specialization in OutputWriter and transliterate_core loop |
| 12 | Evaluate packed `TokenUnit` | experiment with 32/64-bit packed representation for equality/hash/cache locality | Tamil lookup/hashing; size | ⬜ experimental | |
| 13 | Benchmark Wasm size vs speed profiles | compare current `-Oz` with `-O3` + supported LTO/profile choices without mixing compiler flags into core refactors | Wasm throughput vs bytes | ⬜ pending | |

## Step boundaries and acceptance criteria

### 0B — baseline

Capture `00-baseline` from a clean commit before Step 1. Keep the snapshot artifacts locally for the duration of the tuning program. Record compiler/Emscripten versions in the associated PR or working notes if the toolchain changes during the program; comparisons across different toolchains should not be treated as equivalent.

### 1 — buffer ownership

This is a correctness prerequisite, not a speed optimization. Preserve `malloc/realloc` growth semantics and use a matching `free` deleter. The step is complete when the allocator/deallocator mismatch sanitizer suppression can be removed.

### 2 — cold-start benchmark

The existing short-call benchmark deliberately warms the process, so it does not measure the initialization cost targeted by generated metadata/tries. Add a process-isolated benchmark that performs one transliteration and exits. Extend the progression report with cold p50 once this metric exists.

### 3 — Tamil traditional map

Make no other writer refactors in the same PR. The output for TamilTraditional cases must be identical.

### 4 — stack reader/writer

Only remove the reader/writer object allocations. Do not combine with streaming input or cache redesign.

### 5 — writer map views

Prove `ScriptWriterMap` needs no ownership while the existing `ScriptInfo` lifetime is still unchanged. This prepares the later constexpr metadata switch with minimal risk.

### 6A/6B — constexpr script metadata

Split generation and consumption. First emit and parity-test the new tables without changing runtime behavior. Only after parity is established should the runtime `ScriptData` parser/maps be deleted.

### 7A/7B/7C — generated flat tries

Keep data-structure implementation, data generation/parity checking, and runtime cutover separate. Generated children must be sorted and longest-match semantics must match the existing dynamic trie before the old trie is removed.

### 8 — `ScriptId`

Resolve source/destination strings once at the public boundary. Do this after generated metadata and tries exist so the mutable reader/writer caches can be removed rather than optimized in place.

### 9 — streaming input

This is a higher-correctness-risk change. Preserve XML/protected-span handling, equivalents/extra tokens, Tamil superscript/subscript handling, and the existing one-token lookahead semantics. Peak live heap and allocation bytes on large inputs are the primary metrics.

### 10 — sink API

Keep the existing convenience API. Introduce a lower-level sink/output path so callers that own buffers can avoid the final result allocation. Do not require every binding to adopt the new path in the same PR.

### 11 — script-type specialization

Specialize by the small behavioral categories (`Indic`, `Tamil`, `Latin`), not by every script pair. Reject the change if the Wasm/code-size increase outweighs the measured runtime benefit.

### 12 — packed `TokenUnit`

Treat this as an experiment. Merge only if measurements show a meaningful benefit without obscuring correctness or significantly increasing code complexity.

### 13 — Wasm profiles

Keep compiler/profile comparison separate from source optimizations. Report speed and raw Wasm size for each candidate profile; deployment may keep a size-oriented and a speed-oriented profile if both have a concrete use case.

## Stop/go rule

After each optimization, compare the cumulative report and the PR-local regression report. If a proposed optimization adds complexity but produces no repeatable benefit in its target metric, do not merge it merely because it matches the architectural direction. Record the finding and move to the next step.
