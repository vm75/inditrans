# Compact static script data experiment

## Decision and checkpoint

**Promising but needs more optimization and steadier latency measurement.**
The recommended experimental checkpoint is `b62a48737516dd53be67bc23f404e9b466f7cc83`
on `experiment/compact-static-script-data`, based on clean Step 10 `556cb5f`.
It preserves Step 9 streaming and Step 10 sinks; `inditrans.cpp`, `inditrans.h`,
and `exports.h` are unchanged from Step 10. No Step 11 specialization is included.
Keep Step 10 as the production recommendation until short-call and cold latency
pass a reliable comparison. No claim of a clear performance replacement is made.

The central question is partly answered: readable typed metadata can compile
entirely into smaller immutable runtime structures without metadata construction
or additional allocations. The result is **96.09 KiB**, a **30.81%** reduction
from fresh Step 10, with its exact allocation behavior. It does not reach the
59–65 KiB target. The full throughput comparison has zero regressions at the
5% threshold; short-call means and tails remain unstable between repeated blocks.

## 1. Architecture

Canonical JSON still feeds the Python generator. `script_data.h` declares UTF-8
source spellings, named `ScriptToken` sequences, masks, exceptional alternatives,
writer pool entries, aliases, and Tamil prefix strings. `char8_t` conversions
are isolated in compile-time key helpers; runtime compares unsigned bytes.

Compilation performs sequence resolution, source-terminal extraction, full
scratch byte topology construction, reachability packing, and dispatch-page
deduplication. Only reachable branching states and accepted prefixes with
children survive as nodes. A high-bit dispatch word identifies a terminal leaf
directly; Roman payloads contain their sequence ID. Nonterminal states bypassed
by a compressed path, leaf nodes, dense-node edge copies, and compressed fallback
edges are omitted. A failed/truncated compressed path retains the last accepted
prefix; there are no skipped intermediate terminals.

One physical non-Roman graph serves explicit readers and virtual `indic`.
Overlapping Roman meanings retain separate normal/folded dictionaries. Dense,
two-byte, and three-byte dispatch share deduplicated 64-slot pages. Sparse edges
use a shared search helper. Runtime code has four reader policies, with no
per-script generated executable matcher. Leaf payloads use terminal-indexed
fields; branching payloads and compressed paths use compact-state indexing to
avoid dependent loads. This spends some bytes to protect runtime performance.

Writer strings retain the existing deduplicated UTF-8 pool and checked compact
offset/length tables. Sequence tokens and rare alternatives are shared static
arrays. Tamil strings are grouped into token keys at compile time and retain the
small generic token trie. No runtime string preprocessing, dictionary builder,
cache, mutable metadata, writer binding, or initialization guard is introduced.
Input windows and output allocations remain those of Step 10.

## 2. Implementation and investigated size sources

| File | Change |
|---|---|
| `native/src/packed_trie.h` | Reachability packing, encoded terminal leaves, pooled pages, bounded generic matching |
| `native/src/static_trie.h` | Exact compressed-path storage for scratch/token graphs; shared empty triple page map |
| `native/src/static_script_types.h` | Typed source records, canonical sequence resolution, UTF-8 writer validation, probe accessor |
| `native/src/static_scripts.h` | Terminal fields, parallel branching fields, and packed reader descriptors |
| `tool/python/static_scripts.py` | Generate readable spellings with semantic payloads and compile-time transformations |
| `native/src/script_data.h` | Regenerated source declarations and packed table construction |
| `native/tests/test-packed-scripts.cpp` | Exhaustive bounded packing comparisons and 32-bit leaf/index fixture |
| `Makefile` | Disable C++ modules explicitly for the existing Boost.UT header under Clang 23; benchmark flags unchanged |
| Flutter/JS/Node Wasm assets | Rebuilt from the same engine; both JS copies agree |

Inspection covered the original `0898c5d`, typed `b59f157`, Step 6 `1395611`,
Step 9 `c653b49`, Step 10 `556cb5f`, and Step 11 `02e7592` using historical
headers/diffs, native objects, Wasm sections, link maps, and generated code.
Typed metadata achieves its small image by rebuilding dictionaries at runtime;
that trade-off was not imported into the static implementation.

Step 10's cost is predominantly expanded static topology/payloads, rather than
readable UTF-8 literals or executable specialization. Its shared byte graph has
2,296 nodes, including 1,310 leaves and 513 compressed paths. It reserves path
records for every node (18,368 native bytes) and keeps ordinary edges beside
accelerators. Leaf node records, bypassed path states, redundant edges, dispatch
pages, and expanded state-indexed terminal fields account for the main savings.
The final shared graph has 508 node slots including the root.

Compiler/linker inspection matters: Wasm packs nonzero data segments, so merely
removing large zero-filled arrays saves relatively few serialized bytes.
Source spelling records and constexpr scratch objects were already discardable.
Packing meaningful nonzero topology/payloads gives the larger reduction.
Native object inspection quantifies the major table contributors (not directly
comparable to serialized Wasm segment bytes):

| Static objects | Step 10 bytes | Candidate bytes |
|---|---:|---:|
| Shared non-Roman trie | 56,092 | 15,716 |
| All ten Roman/folded tries | 57,636 | 29,100 |
| Source selection arrays, including final parallel branch fields | 27,552 | 24,996 |
| Source-prefix masks | 4,096 | 4,096 |
| Writer UTF-8 pool | 6,705 | 6,705 |
| Writer offset/length entries | 10,808 | 10,808 |


Native `-O3` also inlined sparse search into all four policies in an early packed
variant; sharing that helper reduced each policy by roughly 400–500 code bytes.
Parallel payload/path arrays remove dependent loads added by the smallest layout.

## 3. Wasm size and compiler output

| Artifact | Bytes | KiB | Candidate delta |
|---|---:|---:|---:|
| Typed metadata `b59f157` | 60,324 | 58.91 | +38,075 B (+63.12%) |
| Step 6 `1395611` | 143,875 | 140.50 | −45,476 B (−31.61%) |
| Step 9 / historical Step 10 | 142,228 | 138.89 | −43,829 B (−30.82%) |
| Fresh Step 10 | 142,222 | 138.89 | −43,823 B (−30.81%) |
| Candidate `b62a487` | **98,399** | **96.09** | — |

The authoritative historical sizes are retained. Fresh Step 10 rebuilds six
bytes smaller with the current toolchain; candidate section comparisons use
that fresh artifact. Step 10's checked-in asset is 143,868 B; it is not used
as the measured Step 10 artifact. Both rebuilds use Emscripten 6.0.10 and the repository's standalone
`-Oz -DNDEBUG -fPIC` flags, no exceptions/RTTI, and normal section GC.

| Wasm section payload | Fresh Step 10 | Candidate | Delta |
|---|---:|---:|---:|
| Code | 27,482 B | 27,435 B | −47 B |
| Data | 114,360 B | 70,573 B | −43,787 B |

Section framing and the other small sections explain the remaining difference.
These are serialized section payloads, not native symbol sizes or runtime RSS.
Both release images import nothing. `__wasm_call_ctors` contains only `nop`.
Native symbols and final Wasm link maps contain no readable source records,
canonical lookup, sequence pool object, source terminal objects, full original
byte tries, packing intermediates, scratch graph, global metadata initializer,
or guard variables. Compact tables, sequence tokens, alternatives, writer text,
and writer tables are the intended surviving data. The generic Tamil trie is
also intentional.

**Readability costs exactly 0 bytes.** Pre-readable `4e16dc7` and final standalone
Wasm are byte-identical, SHA-256
`a6ced520733dc6d7563cca5bca01b8299610a22a4a4ef189fe07d3ba0c2fa06b`.
The native throughput executable is also byte-identical before/after the
readability change. Both JS assets are 129,671 B (previously 191,990 B).

Single translation-unit native `-Oz` compile observations: Step 10 2.807 s /
222,404 KiB peak compiler RSS; candidate 4.054 s / 268,216 KiB. These are
host-specific observations, not repeated build benchmarks. Compile-time work
increased, but needs no raised constexpr limits or template-per-script code.

## 4. Memory

Linux/glibc probe values for the primary ~1 MiB Devanagari workload. Allocation
events count `malloc + calloc + realloc`; peak means requested live bytes, not
process RSS. Historical and new measurements have identical workload sizes.

| Version | Cold events | Warm events | Cold allocated B | Warm allocated B | Cold peak B | Warm peak B | Cold live B | Warm live B |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Typed metadata | 1,091 | 3 | 27,307,502 | 27,265,344 | 26,252,348 | 26,216,676 | 35,672 | 0 |
| Step 6 | 3 | 3 | 27,265,344 | 27,265,344 | 26,216,676 | 26,216,676 | 0 | 0 |
| Step 10 | 7 | 7 | 1,050,181 | 1,050,181 | 1,049,821 | 1,049,821 | 0 | 0 |
| Candidate | 7 | 7 | 1,050,181 | 1,050,181 | 1,049,821 | 1,049,821 | 0 | 0 |

All six candidate modes equal Step 10 **byte-for-byte**, cold and warm:

| Mode | Events | Allocated B | Peak B | Live B |
|---|---:|---:|---:|---:|
| devanagari | 7 | 1,050,181 | 1,049,821 | 0 |
| latin | 9 | 7,341,708 | 6,292,364 | 0 |
| virtual-indic | 7 | 1,050,181 | 1,049,821 | 0 |
| expansion | 8 | 3,147,269 | 3,146,525 | 0 |
| protected | 7 | 1,050,113 | 1,049,753 | 0 |
| mixed-protected | 7 | 1,050,180 | 1,049,820 | 0 |

The main peak is ~1 MiB; Latin and expansion modes still peak at ~6 MiB and
~3 MiB respectively. There is no universal 1 MiB bound. The isolated reusable
metadata/readers/writers/Tamil probe reports zero allocation/free events, live,
peak, and untracked bytes, checksum 1047. No typed-metadata first-call allocation
burst or retained metadata heap returns.

## 5. Cold start

Each entry is **p50 / p95 in µs**, from 501 fresh processes per case. Process
launch and returned-string destruction are outside the timed region.

| Path | Historical Step 10 | Fresh Step 10 (31) | Candidate (27) | Typed metadata | Step 6 | Original baseline |
|---|---:|---:|---:|---:|---:|---:|
| devanagari-to-telugu | 6.368 / 8.659 | 6.157 / 8.876 | 6.949 / 9.743 | 42.365 / 52.078 | 6.030 / 7.249 | 143.341 / 182.867 |
| iso-to-devanagari | 7.154 / 9.167 | 6.883 / 9.217 | 7.189 / 11.246 | 53.264 / 78.658 | 6.496 / 8.051 | 158.714 / 206.058 |
| devanagari-to-tamil | 7.362 / 8.683 | 7.184 / 12.159 | 8.156 / 10.893 | 43.548 / 67.728 | 7.062 / 8.232 | 144.852 / 194.614 |
| indic-to-iso | 6.058 / 7.725 | 6.096 / 8.213 | 6.738 / 9.076 | 177.256 / 227.584 | 5.515 / 6.946 | 262.689 / 364.610 |

The candidate retains the static first-call architecture and removes typed
metadata's 42–177 µs p50 initialization cost. Relative to fresh Step 10 (31),
its p50 is approximately 4–14% higher. Another clean Step 10 block (30) measured
7.753, 8.284, 8.231, and 6.247 µs: the comparison changes direction on three
paths. This supports Step-6-class cold-start behavior, but does not establish
cold-latency parity or a speedup against Step 10. A reliable replacement gate
must resolve this variation rather than assuming it is harmless.

## 6. Throughput and short-call latency

Host: Intel Core Ultra 9 185H, Linux 6.18.40.1 WSL2 x86-64, CPU 0; Clang
23.1.1, native `-std=c++23 -O3 -DNDEBUG`; Emscripten 6.0.10. Same scripts,
options, generated data inputs, benchmark corpora, flags, and release semantics
as the authoritative history. Every compared output hash and size agrees.
No dirty `current` or dirty compact-experiment row is used as a baseline.

Primary comparison alternates the **existing** repository `bench-save` commands
between two clean worktrees for five rounds, reversing order each round. The
30-case throughput matrix and ten short cases each have five complete runs
per engine. Aggregation uses the repository arithmetic mean of per-run timing
statistics, retaining all raw runs. Additional clean five-run snapshots show
8–10% reference drift, motivating this alternating comparison. Alternation
between full processes reduces drift but does not eliminate case-level outliers.

Negative percentages mean faster. Improvements/regressions below are classified
at ±5% before rounding; smaller differences are approximately unchanged.

| Input | Improved | Unchanged | Regressed | Median change | Worst change |
|---|---:|---:|---:|---:|---:|
| short | 0 | 10 | 0 | -1.5% | +2.0% |
| ~4 KiB | 1 | 9 | 0 | -2.0% | +2.0% |
| ~1 MiB | 0 | 10 | 0 | -0.4% | +1.2% |

| Case | Short | ~4 KiB | ~1 MiB |
|---|---:|---:|---:|
| indic-to-indic | -4.3% | -6.4% | +0.1% |
| tamil-output | -2.2% | -0.7% | -3.9% |
| latin-input | +0.0% | -2.0% | -2.3% |
| virtual-indic-to-latin | -1.8% | -2.4% | -0.2% |
| virtual-indic-to-indic | +2.0% | +2.0% | -0.7% |
| virtual-indic-to-tamil | -2.8% | -2.0% | -4.1% |
| latin-output | -0.7% | -2.1% | +1.2% |
| expansion-heavy | -1.5% | -5.0% | -1.8% |
| protected-spans | -1.5% | -0.2% | +0.1% |
| mixed-protected-spans | -1.1% | -0.3% | +0.8% |

The strongest throughput change is indic-to-indic at ~4 KiB (−6.4%). The largest
positive changes are virtual-indic-to-indic short/+4 KiB (+2.0%) and latin-output
~1 MiB (+1.2%). Latin input, expansion, and protected spans show no >5%
throughput regression against the contemporaneous reference.

Historical comparisons use the clean five-run records from `out/report.md`.
Each cell below gives **median change / worst change (case)** across ten cases.
These separate older blocks are context, not an alternating performance gate:

| Historical reference | Short | ~4 KiB | ~1 MiB |
|---|---:|---:|---:|
| Typed metadata | -8.7% / +0.3% (mixed-protected-spans) | -1.6% / +1.8% (tamil-output) | -3.6% / +2.7% (mixed-protected-spans) |
| Step 6 | -5.1% / +5.8% (latin-input) | -6.5% / +10.0% (protected-spans) | -8.2% / +12.8% (protected-spans) |
| Step 9 | +0.4% / +6.6% (protected-spans) | +3.8% / +11.2% (protected-spans) | +1.4% / +11.1% (protected-spans) |
| Historical Step 10 | -1.2% / +1.3% (virtual-indic-to-indic) | +0.5% / +2.4% (virtual-indic-to-indic) | -0.1% / +3.3% (protected-spans) |

Protected spans remain faster in the older Step 9/Step 6 records, by up to
11.1%/12.8% on large inputs. The scanner/protected-span implementation is
unchanged from Step 10; historical timing drift does not justify claiming all
Step 9 throughput is retained. The fresh Step 10 comparison is the architectural
gate for this branch.

Short-call timing includes destruction and measures 10,000 calls per case per
run. Here are **all** cases; p50 values are nanoseconds. Both blocks use five
alternating runs, and every value is the untrimmed repository mean:

| Case | Primary ref → candidate p50 | p50 Δ | p95 Δ | Confirmation ref → candidate p50 | p50 Δ | p95 Δ |
|---|---:|---:|---:|---:|---:|---:|
| indic-to-indic | 761 → 731 | -3.9% | -21.3% | 717 → 937 | +30.7% | +18.2% |
| tamil-output | 885 → 998 | +12.8% | +35.9% | 935 → 882 | -5.7% | -13.8% |
| latin-input | 831 → 744 | -10.5% | -14.2% | 740 → 727 | -1.8% | -14.8% |
| virtual-indic-to-latin | 756 → 716 | -5.3% | -4.6% | 693 → 696 | +0.4% | +1.0% |
| virtual-indic-to-indic | 699 → 718 | +2.7% | +18.5% | 703 → 705 | +0.3% | -17.0% |
| virtual-indic-to-tamil | 907 → 897 | -1.1% | +17.1% | 862 → 887 | +2.9% | +22.2% |
| latin-output | 739 → 725 | -1.9% | +24.7% | 714 → 721 | +1.0% | +10.6% |
| expansion-heavy | 187 → 183 | -2.1% | +27.4% | 150 → 149 | -0.7% | +21.0% |
| protected-spans | 293 → 280 | -4.4% | +19.8% | 276 → 327 | +18.5% | +17.9% |
| mixed-protected-spans | 622 → 627 | +0.8% | +1.4% | 632 → 629 | -0.5% | +17.1% |

Primary Tamil p50 +12.8% is driven by a candidate run at 1,387 ns, with the
other four at 933/899/882/891 ns versus reference 883–888 ns. Confirmation
Tamil improves −5.7%, but candidate indic-to-indic has 1,048/1,434 ns outliers
and protected spans has a 590 ns outlier. Those reversals and noisy p95 values
prevent a clear replacement decision. They are retained and disclosed, not
removed as noise. Further latency work should first stabilize the measurement
environment, then investigate any remaining repeatable slowdown.

These are native runtime measurements. Wasm size, constructor absence, exports,
and functional behavior were checked; this experiment makes no claim about
Wasm throughput parity from native timings.

## 7. Readability

Previously a spelling's payload and trie entry were separated by thousands of
lines and opaque numeric sequence IDs:

```cpp
/* u8"क" */ {scriptMask(ScriptId::Devanagari), 4133, 4133, {0, 0}},
// ... separate reader entry ...
{u8"क", 37},
```

The generated source now associates spelling and meaning directly:

```cpp
{u8"क", {scriptMask(ScriptId::Devanagari),
           sequenceId(seq(consonant(0, ScriptType::Indic))),
           sequenceId(seq(consonant(0, ScriptType::Indic))), {0, 0}}},
```

Writer entries similarly change from a comment plus `{1142, 3}` to
`writerChar<writerText>(u8"क", 1142)`, validating literal bytes and length at
compile time. Roman mappings and exceptional alternatives name full token
sequences; canonical boundary resolution preserves existing sequence IDs.
The canonical sequence pool and six Tamil UTF-8 prefix strings were already
semantic/readable and remain so. Flat trie relationships are derived from
spellings, not manually emitted numeric node listings. Source declarations
shrink from 9,858 to 8,323 header lines, while emitted runtime bytes stay equal
to the pre-readable candidate. The JSON inputs remain authoritative.

## 8. Stages and trade-off decision

| Checkpoint | Change | Standalone bytes |
|---|---|---:|
| `556cb5f` | Fresh Step 10 | 142,222 |
| `418250b` | Exact path storage / empty maps | 140,057 |
| `ab844e1` | Reachable nodes, terminal leaves, compact payloads | 97,533 |
| `abf4793` | Pool equivalent dispatch pages | 94,362 |
| `1692272` | Parallel branch payloads | 97,755 |
| `b0a98dd` | Shared sparse search helper | 97,755 |
| `4e16dc7` | Parallel compact-state paths | 98,399 |
| `0f4b1cd` / `b62a487` | Readable declarations / probe and Node asset fix | 98,399 |

Significant stages were committed and measured individually with clean five-run
snapshots. The smallest checkpoint spends fewer bytes on parallel fields but
adds dependent loads; native compiler output also exposed excessive sparse
search inlining. The final representation deliberately spends 4,037 B over
that smallest checkpoint on runtime layout. This is a performance decision,
not a readability surcharge.

Priority assessment:

1. **Performance/memory:** exact Step 10 allocation behavior and static metadata;
   full fresh throughput matrix passes the 5% gate. Short/cold latency cannot yet
   be approved from these variable measurements. Preserve this as an experiment.
2. **Size:** a useful 30.81% reduction, still 38,075 B larger than typed metadata.
   Static expanded dispatch and parallel fields explain much of the remaining
   cost; reaching 59–65 KiB was not demonstrated.
3. **Readability:** materially clearer spelling-to-token declarations, checked
   UTF-8 entries, no emitted duplicate source representation, zero byte cost.

Recommended experimental/default checkpoint: **`b62a487`** (also best-readable).
Smallest measured checkpoint: **`abf4793`**, 94,362 B; it is not recommended over
the final runtime layout. No best-performance claim is made from inconsistent
short-call blocks. Step 10 remains the production default pending that gate.

## 9. Validation and reproduction

Completed on the final engine checkpoint:

- `make test`: all C++ suites pass, including 384,942 packed-data assertions.
  Differential packing tests cover every non-Roman spelling/source, rejected
  terminals, virtual policy, every Roman/folded graph, every bounded prefix,
  unmatched suffixes, and explicit 32-bit terminal values 32768/40000/65535.
- `dart analyze`: no issues; `flutter test`: 47 tests pass against rebuilt native
  shared library; `yarn test`: one suite, 47 tests pass against rebuilt JS/Wasm.
- `make validate`, generator determinism, standalone Wasm smoke, and equality
  of the two JS distribution assets pass. The C ABI and script enums did not
  change, so FFI bindings need no regeneration.
- `make bench-lookup-alloc-linux`: zero reusable-metadata allocations.
- Release native object, final Wasm map/WAT, section sizes, initialization
  symbols, and identical pre-/post-readability binaries inspected.

The existing ts-jest configuration emits its TS151002 warning; tests pass.
Compiler versions actually exercised are recorded above; no new claim of
minimum-version, Windows runtime, sanitizer, or Wasm performance coverage is made.

Raw results remain ignored local artifacts under `out/perf/` and `out/paired/`:
`20-step10-reference` and `21`–`27` stage snapshots; `30`/`31` repeated references;
`32-step10-paired` / `33-candidate-paired` final throughput and short-call blocks;
`34-step10-short-confirm` / `35-candidate-short-confirm` extra latency blocks.
The latter paired records contain five native runs and allocations; they are
not cold/Wasm snapshots. Cold and size results come from the explicitly named
complete snapshot 27 and reference 31. Every final timing record has clean Git
metadata. Historical CSVs remain under `out/history/`; the original
`out/report.md` is preserved unchanged.

To reproduce complete snapshots with the configured toolchain:

```bash
make perf-snapshot PERF_NAME=27-readable-static BENCH_RUNS=5 BENCH_CPU=0 PERF_CPU=0
make bench-short-repeat PERF_REPEATS=5 PERF_CPU=0
make bench-cold COLD_BENCH_SAMPLES=501 PERF_CPU=0
make bench-lookup-alloc-linux
```

For alternating worktree runs, use `make bench-save BENCH_RUNS=1 BENCH_CPU=0`
with distinct `BENCH_BASELINE` paths, alternating clean Step 10 and candidate
order over five rounds. Aggregate with the repository's arithmetic mean and
retain every run/hash. Local orchestration and inspection commands are saved
beside the results. Rebuild using the existing repository flags and all scripts.

[Architecture](../ARCHITECTURE.md) and
[static lookup design](static-lookup-design.md) describe the resulting runtime.
