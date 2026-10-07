# Performance baseline

This document is the current reproduction guide for performance and resource
measurements. Native timings are host-specific; use the saved hashes and
allocation counts as correctness/resource checks and compare timings only on
the same toolchain and machine.

## Reproduce

The authoritative clean-tree baseline is commit
`d17280a207e008c5a4f5ace6705424c01f8c20b4` on
`experiment/compact-static-script-data`, captured 2026-10-07 UTC. The selected engine checkpoint is `f79e14e4d49aa978ba866b9688da3a70e41bc6a2`.
The baseline includes compile-time portability fixes after that checkpoint; the
runtime lookup and transliteration path remain unchanged.

| Environment | Recorded value |
|---|---|
| OS / CPU | Linux 6.18.40.1-microsoft-standard-WSL2 x86-64; Intel Core Ultra 9 185H |
| Native compiler / library | Clang 23.1.1; libstdc++ `__GLIBCXX__=20260810` |
| Supporting toolchain | GCC 16.2.1; CMake 4.4.4 |
| Wasm compiler | Emscripten 6.0.10 (`d6c521a7`) |
| Language | C++23 |
| Native benchmark flags | `-std=c++23 -O3 -DNDEBUG` |
| Wasm release flags | `-std=c++23 -Oz -fno-exceptions -fno-rtti -fno-stack-protector -ffunction-sections -fdata-sections -fno-math-errno -DNDEBUG` |
| CPU affinity | CPU 0 pinned for benchmark and cold-start processes |
| Repetitions | Five full throughput and short-call sweeps; 501 samples per cold path |
| Environment | WSL2; Linux/glibc allocator probe; timing results are host-specific |

Raw files are retained in [`performance-baseline/`](performance-baseline/).
The throughput capture contains all 30 case/size cells and 150 raw rows with
hashes; all five hashes agree for each cell. The short-call capture has all ten
cases across five runs with stable hashes. Cold output hashes also match the
throughput cases. Cold and warm allocation data cover six workloads. The
isolated reusable-metadata lookup probe reports zero malloc/calloc/realloc/free,
zero live and peak bytes, zero untracked events, checksum 1047.

To reproduce from a clean checkout, use the production Clang flags
(`-std=c++23 -O3 -DNDEBUG`) and Emscripten 6.0.10 release build (`-std=c++23
-Oz -fno-exceptions -fno-rtti -DNDEBUG`). Pinning is recommended on Linux. The
standard snapshot captures five complete throughput and short-call runs, 501
fresh-process cold samples per path, cold and warm allocation probes on
Linux/glibc, output hashes, and standalone Wasm size:

```sh
CPU=$(python3 -c 'import os; print(min(os.sched_getaffinity(0)))')
make perf-snapshot PERF_NAME=00-baseline BENCH_RUNS=5 BENCH_CPU="$CPU" PERF_CPU="$CPU"
```

Raw CSVs and metadata are written under ignored `out/perf/`. Preserve those
files with the build artifact or CI run when publishing a new baseline. The
throughput matrix has ten cases at short, approximately 4 KiB, and 1 MiB input
sizes. Every case/size output hash must match when comparing revisions. Short
call measurements include returned-string destruction. Cold measurements
sample the first transliteration in fresh processes and exclude process launch.
Allocation figures are requested allocator bytes, not RSS or Wasm linear-memory
capacity.

For A/B checks, use `make bench-save` before a change and `make bench-compare`
afterward. The default report threshold is 5%; set `BENCH_STRICT=1` to fail at
or above it. Output/hash mismatches and missing benchmark cases always fail.
Use `make bench-short-repeat` for five full latency sweeps when assessing small
short-call changes. Do not treat sub-microsecond variation on shared hosts as
a stable architectural effect.

For engineering decisions, investigate repeatable whole-call regressions above
approximately 3% on representative hot workloads and repeatable short-call p95
regressions above approximately 5%, after accounting for noise. These are
investigation thresholds, not absolute cross-host CI timing requirements. The
pull-request timing job compares against the clean selected-engine baseline
(`dfe5488`) on a shared hosted runner and uses its existing 10% threshold;
hashes, case completeness, and allocation checks are also enforced. It applies
the current compile-time-only portability helpers to the baseline source so
older baseline code can build with current runner compilers without changing
the runtime reference.
Metadata construction must remain allocation-free. Avoid large increases in
allocation frequency or transient bytes without explicit justification, and
do not regress streaming output toward the former roughly 25 MiB temporary
buffer behavior. The selected 89.73 KiB Wasm is the accepted reference; future
size growth requires justification, not a return to the superseded +5% limit
against the old runtime-built Wasm. Compile-time transformation cost is an
accepted trade-off, though substantial further increases should be justified.

## Final baseline results

Throughput cells below show the median of the ten case medians at each scale;
see the raw CSV for every case, actual input size, p95, returned size, and
checksum.

| Input scale | Median ns/call | Median sample p95 ns |
|---|---:|---:|
| Short | 1,191 | 1,617 |
| Approximately 4 KiB | 45,872 | 60,046 |
| Approximately 1 MiB | 11,812,250 | 13,575,010 |

Short-call p50/p95 are nanoseconds and include returned-string destruction.

| Case | p50 ns | p95 ns |
|---|---:|---:|
| indic-to-indic | 1,239 | 1,648 |
| tamil-output | 1,430 | 2,109 |
| latin-input | 1,153 | 1,344 |
| virtual-indic-to-latin | 1,109 | 1,193 |
| virtual-indic-to-indic | 1,111 | 1,197 |
| virtual-indic-to-tamil | 1,379 | 1,709 |
| latin-output | 1,134 | 1,457 |
| expansion-heavy | 239 | 284 |
| protected-spans | 417 | 447 |
| mixed-protected-spans | 983 | 1,124 |

Cold values are first-call p50/p95 in nanoseconds from 501 fresh processes per
path; process launch is excluded.

| Path | p50 ns | p95 ns |
|---|---:|---:|
| Devanagari → Telugu | 10,227 | 13,676 |
| ISO → Devanagari | 10,112 | 14,142 |
| Devanagari → Tamil | 12,083 | 17,632 |
| Indic → ISO | 9,885 | 12,381 |

Cold and warm allocation results match for every workload. Values are allocator
events, requested bytes, peak requested-live bytes, and retained live bytes.

| Workload | Events cold/warm | Requested B cold/warm | Peak B cold/warm | Retained B cold/warm |
|---|---:|---:|---:|---:|
| devanagari | 7 / 7 | 1,050,181 / 1,050,181 | 1,049,821 / 1,049,821 | 0 / 0 |
| latin | 9 / 9 | 7,341,708 / 7,341,708 | 6,292,364 / 6,292,364 | 0 / 0 |
| virtual-indic | 7 / 7 | 1,050,181 / 1,050,181 | 1,049,821 / 1,049,821 | 0 / 0 |
| expansion | 8 / 8 | 3,147,269 / 3,147,269 | 3,146,525 / 3,146,525 | 0 / 0 |
| protected | 7 / 7 | 1,050,113 / 1,050,113 | 1,049,753 / 1,049,753 | 0 / 0 |
| mixed-protected | 7 / 7 | 1,050,180 / 1,050,180 | 1,049,820 / 1,049,820 | 0 / 0 |

| Release artifact / measurement | Bytes |
|---|---:|
| Standalone Wasm | 91,883 (89.73 KiB) |
| Wasm code section | 27,435 |
| Wasm data section | 64,057 |
| Standalone Wasm gzip (`gzip -n`) | 38,516 |
| JavaScript single-file distribution | 123,155 |
| JavaScript single-file gzip (`gzip -n`) | 45,584 |
| Native Clang release shared library | 198,600 |
| Clean Clang CMake release build | 8.005 s; peak child RSS 319,572 KiB |

The compiler measurement is one clean release build on this host, not a
portable timing target. The Wasm code and data sizes are section payload sizes.

Checked-in historical Wasm sizes provide compact architectural context only;
these artifacts were not rebuilt under the baseline session's toolchain:

| Checkpoint | Checked-in Wasm bytes |
|---|---:|
| Pre-performance `717b766` | 58,479 |
| Step 10 sink API `556cb5f` | 143,868 |
| Compact reference `c3d5480` | 98,399 |
| Selected engine `f79e14e` | 91,883 |

## Selected architecture and trade-offs

The selected implementation is the direct-access compact static design at
engine commit `f79e14e4d49aa978ba866b9688da3a70e41bc6a2`.
Runtime tables retain direct
source masks, explicit and virtual sequence arrays, aligned writer entries,
direct alternative ranges, and packed immutable trie data. There is no runtime
metadata builder, cache, decompressor, or initialization guard. Empty
alternative ranges use canonical zero offsets.

Standalone Wasm is 91,883 bytes (89.73 KiB), with 27,435 bytes of code and
64,057 bytes of data-section storage. This size is accepted for direct immutable
lookup and zero runtime metadata construction. Smaller static layouts were
measured and rejected after repeatable runtime regressions. Compile-time
transformation also increases clean compile time and compiler memory; this is
an accepted build-time trade-off, not an open optimization target.

## Measurement interpretation

The compact-static acceptance measurements recorded five runs of the full
ten-case throughput matrix and short-call suite, all output hashes, 501 cold
samples per representative path, and cold/warm allocation probes for six
workloads. Reusable metadata lookup allocates zero bytes. Whole-call memory
varies by workload: representative peak requested-live values were 1,049,821 B
for Devanagari, 6,292,364 B for Latin, and 3,146,525 B for expansion-heavy
input. These are workload-specific requested allocator peaks, not a universal
engine peak or process RSS.

Treat these measurements as historical architecture acceptance evidence, not
as a replacement for a fresh baseline on the machine and toolchain used for a
change. Do not compare native timing claims with browser/Wasm runtime claims.

## Platform evidence

The acceptance workflow includes Linux native tests and ASan/LSan/UBSan,
Flutter analysis/tests, Emscripten 6.0.10 standalone and JavaScript Wasm builds,
Node tests, MinGW cross-build/export checks, MSVC native and C ABI checks,
Windows PowerShell Wasm builds, Apple Clang native tests plus Flutter macOS/iOS
builds, an Android arm64 Flutter build, and a Chromium browser fixture. The
browser fixture exercises shared cases, repeated calls, and output release.
Configured jobs are not proof of acceptance until their hosted runs pass; see
the current per-platform results in [`platform-support.md`](platform-support.md).
Android runtime remains unexercised unless a device/emulator test is added.
