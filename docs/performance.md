# Performance baseline

This document is the current reproduction guide for performance and resource
measurements. Native timings are host-specific; use the saved hashes and
allocation counts as correctness/resource checks and compare timings only on
the same toolchain and machine.

## Reproduce

Start from a clean checkout and record `git rev-parse HEAD`. Use the production
Clang flags (`-std=c++23 -O3 -DNDEBUG`) and Emscripten 6.0.10 release build
(`-std=c++23 -Oz -fno-exceptions -fno-rtti -DNDEBUG`). Pinning is recommended
on Linux. The standard snapshot captures five complete throughput and
short-call runs, 501 fresh-process cold samples per path, cold and warm
allocation probes on Linux/glibc, output hashes, and standalone Wasm size:

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

The committed CI workflow runs Linux native tests and ASan/LSan/UBSan, Linux
Flutter analysis/tests, Emscripten 6.0.10 standalone and JavaScript Wasm builds,
Node tests, and a MinGW x86-64 DLL cross-build/export check. The DLL runtime
smoke is configured only on push events. CI does not currently establish
macOS, Android, MSVC, or browser runtime acceptance. Flutter declares Android,
iOS, Linux, macOS, Windows, and Web plugin platforms; unsupported or unavailable
runtime checks must be reported as unverified rather than inferred from a
successful build on another host.
