# C++23 and performance implementation plan

- Status: Implementation in progress; acceptance gates remain open.
- Performance baseline: pre-C++23 commit `d8b7140`, inspected 2026-09-26. The
  current `dev`/`main` merge base is `3044baa`.
- Scope: C++ standard adoption, static data, compile-time validation, allocation reduction, and maintainable deterministic processing.
- Background: [C++23 assessment](../../cpp23-performance-exploration.md).

## 1. Outcome and constraints

Adopt **C++23**, the latest published ISO C++ standard at this snapshot, and use selected features to simplify a faster, smaller implementation. Rechecked on 2026-09-26: ISO lists ISO/IEC 14882:2024 as published and its successor DIS as under development. A later standard becoming published is grounds for reviewing this plan, not silently changing its scope. [ISO status](https://www.iso.org/standard/83626.html).

The completed work should:

1. Select and enforce C++23 consistently for native, Flutter platform builds, and both Wasm distributions.
2. Remove runtime construction of built-in script tables and tokenizer/prefix lookup structures.
3. Reduce per-call allocation and excess token-buffer capacity.
4. Separate existing custom handling into small, explicit helpers while preserving observable conversion behavior.
5. Validate generated structure and small derived tables at compile time.
6. Demonstrate improvements against reproducible native/Wasm baselines.
7. Preserve the repository's procedural style, small value types, and direct control flow.

This plan does not implement new pronunciation behavior, models, lexicons, language detection, a new phonological representation, or a new streaming API. Keep the two exploration documents separate and unchanged. Ignore prior GitHub issues and implementation roadmaps.

### Coding rules for every change

- Follow the surrounding naming, indentation, brace style, and header organization. Do not reformat untouched code.
- Use existing classes and functions where their responsibilities remain appropriate.
- Prefer normal loops, named helpers, `std::array`, spans/views, and explicit state.
- Introduce a template only when it removes real repeated logic or measured runtime work.
- Limit specialization to a few behavior families; keep script data selectable at runtime.
- Avoid a generic rule interpreter, plugin registry, recursive template framework, or broad class hierarchy.
- Do not replace working code merely to showcase a C++23 feature.
- Keep explanatory comments about invariants and non-obvious behavior; avoid comments that restate code.
- Remove an old implementation only after all its callers have migrated. Do not bundle unrelated cleanup.
- Hand-edit generator inputs/source, then regenerate outputs. Never manually edit generated bindings or script/option declarations.

## 2. Compatibility contract

Preserve token ordering, options, accepted script names, conversion outputs, protected spans, and existing wrapper behavior. Characterize inconsistent behavior before changing internals; do not normalize it accidentally during optimization.

The exported C ABI and public Dart/TypeScript signatures remain unchanged. Rebuild Wasm after engine changes even when its ABI is stable. Regenerate FFI bindings only if the exported header actually changes.

### Ownership contract

The current builder allocates with `realloc`, but `releaseBuffer()` uses `delete[]`; C++ overloads use `std::unique_ptr<char>` with its scalar default deleter. The existing pointer type cannot express the required array/free ownership contract. The Dart wrapper also does not explicitly release the returned native buffer after copying.

Preferred correction: retain the builder's malloc/realloc storage, use `free` consistently through a named custom-deleter owner, and release Dart's returned buffer in a `finally` block after conversion. The C ABI can stay unchanged.

Replacing the `std::unique_ptr<char>&` overload declared in `native/src/inditrans.h` changes the C++ source interface. The user explicitly approved this change. It is documented in `README.md` and `docs/release.md`; the C ABI and Dart/TypeScript APIs remain stable. Do not add a compatibility adapter that preserves the allocator mismatch.

The user also explicitly approved raising the CMake minimum to 3.20 and documenting the Flutter consumer compatibility change. `flutter/README.md` states that source builds require CMake 3.20 or newer and a C++23-capable toolchain; consuming prebuilt binaries is unaffected. Android's plugin build provisions CMake 3.22.1.

## 3. Target shape

```text
tool/script_data.json and existing constants
                |
        existing Dart generator
                |
   generated arrays, descriptors, trie edges
                |
     C++ constexpr validation/static_assert
                |
   immutable script/token lookup views
                |
   existing reader -> small policy helpers -> writer
                |
      existing native and Wasm C ABI
```

Keep indexed `Token`, `ScriptToken`, and `TokenUnit` concepts. Use fixed-width integers for generated references with validated bounds, not packed native object layouts as a serialization format. Preserve current Unicode matching and equivalent-expansion semantics.

Small tables can be constructed directly by constexpr/consteval functions. Large script/trie structures should be produced by the existing generator and checked by C++; every consumer build should not have to run a large compile-time parser or minimizer.

## 4. Work packages and dependency order

| Package | Deliverable | Depends on |
|---|---|---|
| P0 | Behavioral characterization and reproducible measurement setup | None |
| P1 | Correct output ownership and safe buffer handling | P0; C++ API decision |
| P2 | Consistent, validated C++23 toolchain configuration | P0; support-floor decision |
| P3 | Small per-call allocation reductions | P1, P2 |
| P4 | Generated immutable script descriptors and writer views | P2, P3 |
| P5 | Immutable tokenizer implemented; throughput and Wasm-size gates open | Ranked root bitmaps with exact root-map and child-sequence interning, sorted non-root edges, 4-byte `ReaderNode`, 6-byte `ReaderData`, BMP-bound generator. The 1,048,971-case spelling/options/output differential and 19,200 uppercase ISO/IAST folding cases ran before the final root interning; native tests and fresh benchmark hashes match after it. Three fresh CPU-pinned pairs against a newly compiled `d8b7140` baseline show 1 MiB changes of -1.4% Devanagari→Telugu, -9.3% Tamil, +1.9% Latin input, +4.6% virtual Indic, +2.0% expansion-heavy, -2.3% protected spans, and +1.2% mixed protected spans. Virtual Indic misses the 3% gate. Six pairs versus the exact-root-only predecessor kept all long-input medians within 3%; ten short-call pairs kept p95 within -15.90% to +1.57%. Warm requests are 3–5 versus 9–11 baseline; peak heap falls about 30% for Indic workloads. Current standalone Wasm is 57,512 B / 28,617 B gzip and JS/Wasm is 91,715 B / 42,848 B gzip, up 17.7% and 13.9% compressed; data section is 36,965 B vs 17,490 B baseline. Native shared library is 95,128 B vs 135,376 B (-29.7%). P5 is not merge-ready.
| P6 | Minimal extraction of custom handling | P3; finalize after P5 |
| P7 | Reduced input-buffer capacity and optional measured tuning | P5, P6 |
| P8 | Partial; local validation broad, hosted/runtime checks open | On the current exact-child-sequence-interned source, `make test`, CMake Release/CTest, ASan/UBSan (`ASAN_OPTIONS=detect_leaks=0`), direct Dart analysis, release verification, deterministic regeneration of all three generated headers, Node Wasm smoke conversions, and three-pair native benchmarks passed. The 1,048,971-case output differential predates the final exact-root interning; native fixtures and benchmark output hashes match after it. Browser fixtures could not run because the sandbox denied starting a local HTTP server. Flutter tests were not rerun because the Flutter launcher cannot write its SDK cache. `yarn test` remains blocked by ts-jest incompatibility with installed TypeScript 7.0.2. LeakSanitizer, Android runtime, Windows execution, MSVC, Apple Clang/SDK, and hosted plugin jobs remain unverified.

Each package should be reviewable on its own. Keep baseline results, output comparisons, and resource measurements with the implementation review. A package may need more than one commit; no commit should combine a semantic correction with an unrelated optimization.

## 5. P0 — Establish behavior and performance evidence

### Files

`native/bench/benchmark.cpp` — throughput benchmark (median/p95 ns/call, MB/s) across 10 cases × 3 input sizes (32 B, 4 KiB, 1 MiB).
`native/bench/short_call.cpp` — per-call latency distribution (p50/p95 over 10 000 samples) for the same cases at a fixed small input.
`native/bench/memory.cpp` — one-shot memory footprint runner (input/output sizes); doubles as the allocator-event probe binary when built with `DINDTRANSLIT_ALLOC_PROBE`.
`native/bench/allocation_probe_linux.c` — Linux/glibc `LD_PRELOAD` shared library that intercepts `malloc`/`realloc`/`free` and reports counts, total bytes, peak live bytes, and bytes still live after the measured call via `alloc_probe_reset()`/`alloc_probe_report()` symbols.

Makefile targets: `bench` (throughput), `bench-short` (latency), `mem-bench` (footprint), `bench-alloc-linux` (allocator probe), `bench-all` (formatted summary of all four), `bench-save` (save baseline CSVs to `out/`), `bench-compare` (diff current run against saved baseline, flagging regressions ≥ 5%).

All benchmark binaries are built at `-O3 -DNDEBUG`, separate from the existing `-O0 -g` test and CLI builds. Avoid a benchmarking framework dependency.

### Actions

1. Record compiler, standard library, SDK, build flags, revision, and artifact hashes.
2. Run existing native, Dart, and Node tests in their supported environments. Make a missing/unparseable shared fixture fail the native test harness instead of silently running no cases.
3. Characterize aliases, name case, longest-match equivalents, multi-token expansions, `indic`, `FORCE_INDIC` if supported, read/write restrictions, same-script calls, accents, superscripts, and skip spans.
4. Add focused regression cases for paths being changed: overlapping trie keys, terminal nodes with children, prefix failure/reset, output growth, non-ASCII text, end-of-input, and repeated buffer release through supported ownership APIs.
5. Capture custom-handling behavior, including anuswara before vowels/symbols/raw spans and Tamil final-virama handling. Label questionable legacy results explicitly; do not silently replace expected output with a preferred linguistic answer.
6. Compile an optimized benchmark separate from the Makefile's existing `-O0 -g` tests/CLI. Use an output checksum or consumed result to prevent dead-code elimination.
7. Measure cold process/instance startup, first use of a script pair, warmed conversion, and wrapper overhead separately.

### Corpus and measurements

Benchmark cases cover: `devanagari→telugu` (Indic-to-Indic), `devanagari→tamil` (Tamil output), `iso→devanagari` (Latin input), `indic→iso` / `indic→devanagari` / `indic→tamil` (virtual-Indic union reader), `devanagari→iso` (Latin output), expansion-heavy input (`अॅॐऍ`), protected spans, and mixed protected spans. Three input sizes (32 B, 4 KiB, 1 MiB) exercise short-call overhead, intermediate, and sustained throughput separately.

Record for each benchmark run: compiler version, build flags, revision (`git rev-parse --short HEAD`), and output FNV-1a hash (to catch silent correctness regressions). Primary metrics: median and p95 ns/call, MB/s, p50/p95 per-call latency (µs), allocation event counts (malloc/realloc/free), total allocated bytes, peak live heap bytes, and bytes still live after the call returns (non-zero indicates retained caches). Use `bench-save` to freeze a baseline and `bench-compare` to diff; the 5% threshold accounts for run-to-run noise. Wasm memory growth and artifact sizes are measured separately via Emscripten build output and `ls -l`. Keep allocation instrumentation out of production builds.

Existing ownership undefined behavior may make sanitizer runs fail. Record the initial finding, then use the corrected P1 revision as the trustworthy resource baseline. Keep the original baseline for historical comparison, with that limitation clearly stated.

### Exit gate

Fixtures are actually loaded; expected output is frozen; repeated measurements establish a noise range. Baseline failure causes are known. No claim of improvement is made from debug-build timings or a single run.

## 6. P1 — Fix ownership before optimizing allocation

### Files

`native/src/utf.h`, `inditrans.cpp`, `inditrans.h`, affected native tests, and `flutter/lib/inditrans.dart`. Inspect existing JS buffer cleanup; change it only if a concrete failure is found.

### Actions

1. Define one owner for malloc/realloc-backed output with a named `free` deleter; use it consistently in internal return paths.
2. Make the exported `releaseBuffer()` match the allocator. Preserve its symbol/signature.
3. Ensure every successful Dart copy releases the returned pointer, including exceptions while converting it to a Dart string.
4. Avoid pointer subtraction/arithmetic on uninitialized null storage in the builder. Preserve growth behavior unless measurement justifies changing it.
5. Check capacity arithmetic for overflow and avoid losing the original pointer on realloc failure.
6. Define failure handling for output growth under the current `noexcept`/exception-disabled configurations. At minimum prevent corrupt writes; propagate failure to the existing null/empty-result convention where feasible. Do not claim general allocation-failure recovery for the rest of the library.
7. Update in-repo C++ callers to `TranslitBuffer`; document the approved external source migration.

### Exit gate

Native address/undefined-behavior sanitizer checks for the touched paths pass; repeated C ABI and Dart calls do not leak output buffers. Output bytes and C ABI signatures match the characterized behavior. Relevant Dart analysis/tests pass.

## 7. P2 — Adopt C++23 consistently

### Files

`native/src/CMakeLists.txt`, `Makefile`, `tool/build_wasm.sh`, `tool/build_wasm.ps1`, affected Flutter platform build files, `.github/workflows/ci.yml`, and release build configuration where it must match.

### Actions

1. Select exact CI/toolchain versions for a verified feature subset. Record versions for Linux Clang/libc++ or libstdc++, Windows MSVC, Android NDK, Apple Xcode, and Emscripten. Do not infer library support from a compiler accepting the language flag.
2. Use CMake target properties for `CXX_STANDARD 23` and `CXX_STANDARD_REQUIRED YES`; avoid the current ineffective plain `CXX_STANDARD_REQUIRED` variable. Disable extensions only after identifying any existing extension dependencies.
3. Adopt a verified CMake version that recognizes the selected compiler's C++23 mode. Version 3.20 first introduced the standard value; this is a lower bound, not a guarantee for every compiler. Align affected Flutter entry points and document the support change. [CMake standard property](https://cmake.org/cmake/help/v4.0/prop_tgt/CXX_STANDARD.html).
4. Set `-std=c++23` in all direct native/Wasm commands, including the shell standalone release path that currently omits a language mode. Confirm shell and PowerShell paths remain equivalent.
5. Set Apple pod C++ language settings explicitly and verify deployment-target/library compatibility. Do not raise OS deployment targets solely because a newer language mode is selected.
6. Pin Emscripten for CI and reproducible local setup; remove reliance on an unqualified `latest` installation when producing release artifacts.
7. Compile small feature checks for only the adopted features on every toolchain. Prefer compile/link probes over executing cross-compiled programs.
8. Add platform coverage sufficient to verify Windows, Android, Apple, native Linux, and Wasm. Use a pinned validation configuration for Android without unnecessarily forcing all consuming applications to use that exact NDK.

### Feature policy

| Facility | Intended use | Bound |
|---|---|---|
| `constexpr`, `consteval`, `static_assert` | Small derived tables and structural validation | Straightforward loops; no large type-level database |
| C++23 static constexpr locals | Small helper-owned immutable tables | Use where locality improves readability |
| `std::to_underlying` | Touched enum-to-index conversions | Do not churn unrelated casts |
| `std::span` / `string_view` | Views of immutable arrays and bytes | Explicit lifetime; available before C++23 |
| `if constexpr` | A few proven behavior variants | No script-pair Cartesian product |
| `if consteval` | Shared helper with genuinely different compile-time/runtime needs | Use only if such a helper arises; not a mandatory rewrite |
| `std::expected` | Possible new internal validation result | Only if clearer than the existing result convention; no C ABI exposure |

Defer reflection, modules, coroutine generators, generic ranges rewrites, standard flat containers as a blanket replacement, and custom SIMD. `std::string::resize_and_overwrite` is not a drop-in replacement for the owned C buffer. Avoid `assume`/`unreachable` on unvalidated input.

### Exit gate

Every supported build path demonstrably selects C++23; selected features compile on the declared minimum toolchains. Baseline behavior remains unchanged. Record compiler/library versions and support changes before proceeding.

## 8. P3 — Remove small unnecessary allocations

### Files

Primarily `native/src/inditrans.cpp`, with minimal supporting type/helper changes.

### Actions

1. Replace the writer's three-entry traditional-Tamil hash map with a constant array or direct conditional lookup, following existing local style.
2. Resolve reader/writer maps before constructing per-call objects; use automatic storage for `InputReader` and `OutputWriter` where ownership/lifetime permits. Preserve early failure behavior.
3. Avoid copying `ScriptInfo` entries and temporary lookup strings in touched loops. Use references and transparent lookup where practical until P4 removes the relevant maps.
4. Remove constructor work whose computed result is unused, such as `setNasalConsonantSize()`, only after verifying no observable effect.
5. Keep the current output builder and token representation; do not introduce a new allocator framework.

### Exit gate

Reader/writer construction no longer requires separate heap allocations; small substitution lookup allocates nothing. Record the actual per-call reduction and verify short-call performance. Revert any complexity that yields no useful allocation or maintenance benefit.

## 9. P4 — Generate immutable script descriptors

### Files

`tool/utils/script_data.dart`, `tool/generate_headers.dart`, generated `native/src/script_data.h`, and the script map/view portions of `native/src/inditrans.cpp` and `type_defs.h`. Add a small focused header only if it makes ownership of generated views clearer.

### Actions

1. Keep `tool/script_data.json` as the authoring source. Preserve the existing shared token ordering and equivalent definitions.
2. Extend the generator to emit a UTF-8 blob plus fixed descriptors for scripts, categories, aliases, and expansions. Start with 32-bit offsets; narrow individual fields only when measured savings justify it and generation proves the bounds.
3. Deduplicate repeated strings where it reduces total data without complicating lookup. Preserve empty entries and intentional collisions.
4. Replace lazy `ScriptData` parsing with immutable views. Replace writer vectors with views into generated indexed tables.
5. Resolve script names once to descriptors/internal IDs. Preserve supported aliases and characterize case-sensitive special checks before consolidating resolution.
6. Use constexpr validators for category lengths, bounds, alias targets, and expansion references, with meaningful `static_assert` messages.
7. Make generation deterministic: stable ordering, deterministic tie-breaking, no timestamps, and byte-identical regeneration.

### Exit gate

Built-in script metadata and writer tables require no runtime heap construction. Generator equivalence tests and all conversion fixtures pass. Data size, cold startup, and compiler costs are recorded. Do not keep both complete old and new databases in shipped builds.

## 10. P5 — Generate flat lookup structures

### Files

Generator sources; tokenizer lookup in `native/src/utilities.h`/`inditrans.cpp`; prefix lookup in `inditrans.cpp`; `trie.h` only where production callers change. Do not adopt the unused packed trie without validating its design.

### Representation

Use a flat node array and sorted edge arrays. A node identifies its edge range and optional terminal token/expansion. An edge stores a Unicode scalar key and child index. Store references as validated integers, not pointers to separately allocated nodes. The current compact edge format stores scalars in 16 bits and is intentionally BMP-only; generation must fail clearly for any tokenizer key above U+FFFF. Add a compact supplementary-plane escape only if supported script data requires one.

Use an ordinary loop for traversal. For short edge ranges a linear scan may win; use binary search where measurements justify it. Avoid perfect hashing or automaton minimization in the initial implementation.

### Actions

1. Reproduce current longest-match semantics: remember the deepest terminal and its consumed UTF-8 byte length.
2. Resolve multi-token equivalents during generation with the same ordering and conflict behavior as the existing builder. Maintain case-folding scope exactly; do not introduce universal normalization.
3. Generate per-reader roots and the virtual `indic` root. Preserve its current constituent-script insertion precedence and handling of equivalents rather than assuming union of all standalone tries is equivalent.
4. Validate all offsets, sorted edges, terminal token fields, expansion references, and root IDs at generation/compile time.
5. Generate the small Tamil prefix structure using the same normalized TokenUnit keys and equality semantics. Preserve its match/state behavior, including last accepted prefixes and resets.
6. Remove built-in reader caches and production heap-trie construction once all readers use immutable roots. Retain generic trie utilities only if another real caller needs them.
7. Share common table data only where it keeps the generator and lookup readable. Do not trade a simple representation for compression that materially slows lookup.

### Exit gate

No built-in tokenizer or Tamil-prefix node allocation occurs at startup or first conversion. Compare old/new lookup outputs over every generated spelling, overlapping prefixes, representative concatenations, aliases, and unmatched input. The conversion corpus must remain byte-identical.

Review code/data size together: replacing a compressed blob with flat structures can grow the binary. If footprint gates fail, adjust the layout before making the flat tables the default; retain the zero-runtime-construction objective.

## 11. P6 — Extract custom handling with direct calls

### Files

`native/src/inditrans.cpp` and, if justified, one focused header for deterministic handling. Preserve current names and state types wherever practical.

### Actions

1. Extract anuswara selection from output emission: a helper determines the existing selected index/action; the writer encodes it for the target.
2. Move Tamil context/prefix handling into named helpers or a small state struct. Keep explicit superscript/mark decoding distinguishable from inferred transformations.
3. Keep Gurmukhi adhak expansion in source decoding; keep target substitutions and multipart vowel/accent ordering close to the relevant encoder.
4. Pass current/next units and state explicitly. Keep one fused traversal; introduce no event heap, virtual callback chain, or full-text pass.
5. Initially retain current family dispatch. Consider at most a few compile-time loop variants only after P7 measurements identify branch overhead worth removing.
6. Preserve questionable legacy behavior during extraction. Any correction to anuswara's missing token-category check or Tamil consonant suppression requires a separately reviewed behavior change.

### Exit gate

Generic iteration/output plumbing no longer contains the detailed anuswara choice or Tamil contextual algorithm. Direct helper calls preserve output and allocation counts. Source review confirms responsibilities are clearer; compiler output/benchmarks confirm no material dispatch regression.

## 12. P7 — Reduce transient memory and tune only measured costs

### Required input-buffer change

Replace the input-byte-count reservation with a capacity policy based on actual token demand. The smallest initial candidate is an exact counting scan using the same tokenization routine with a counting sink, followed by the existing materialization scan. This lowers excess capacity but adds lookup work; it is not automatically the selected implementation.

Compare that candidate against a modest initial reservation with bounded geometric growth. Choose the simplest policy that reduces peak memory on the declared corpus without violating throughput/latency gates. Account for alias expansion, raw spans, and reallocation peak memory. Do not introduce a separate approximate tokenizer whose behavior can diverge.

If neither approach meets both gates, evaluate a bounded-lookahead internal scanner that preserves the public whole-string API and custom delimiter semantics. This is an escalation in implementation scope, not the default. It requires explicit handling of expansion queues, marks, raw span lifetimes, and prefix state; keep it in a separate review.

### Optional tuning, only with evidence

- Select a small loop variant once per call if it measurably removes repeated branches without excessive code growth.
- Compare release optimization flags and LTO under the same toolchain. Apply flags only to relevant targets; do not impose them on consumers globally.
- Add a cheap common-input fast path only if the measured corpus shows it matters and its fallback preserves all semantics.
- Retain scalar portable C++ unless a specific numeric/decoding bottleneck justifies a later SIMD task.

### Exit gate

Peak transient memory falls on representative long inputs and expansion-heavy input remains bounded/correct. Report allocation frequency as well as capacity: fewer reserved bytes must not disguise a large increase in repeated allocations. Optional tuning is retained only when its benefit exceeds benchmark noise.

## 13. P8 — Integration, documentation, and final acceptance

### Required checks

- `make test`, with verified fixture loading.
- Build the native library, then `dart analyze` and `flutter test` in `flutter/` with the library available.
- Rebuild both Wasm distributions through their respective build paths; run Node's `yarn test`.
- Run browser Wasm conversion checks, including repeat calls, protected spans, and output ownership. Node passing does not prove browser compatibility.
- Build/validate Android and Apple native integrations and Windows; record toolchain versions. Exercise platform runtime tests where runners are available; unavailable checks remain incomplete evidence.
- Run sanitizer checks on touched native memory paths and compare defined-behavior fixtures across the supported targets.
- Regenerate data twice to prove deterministic output. Regenerate bindings/Script/Option outputs when their authoritative inputs change.
- Run `dart tool/verify_release.dart` / `make validate`.
- Inspect exported C symbols, wrapper signatures, generated-artifact changes, and the final diff for unintended behavior or dependency changes.

Some commands need platform SDKs and environment setup. Record exact commands and results during implementation; do not equate planned commands with completed validation. `make testall` currently launches a Flutter example through one target, so use explicit package tests for automated acceptance.

### Proposed performance gates

These are proposed review thresholds, not promised results. Establish baseline noise in P0 before applying them; investigate statistically credible regressions rather than accepting them because they are just under a percentage.

| Metric | Acceptance target |
|---|---|
| Built-in script/trie construction | Zero runtime heap allocations after P4/P5 |
| Reader/writer object construction | Zero separate heap allocations after P3 |
| Output ownership | No leaks/mismatched frees in exercised supported paths |
| Warmed throughput | No repeatable regression greater than 3% on representative cases |
| Short-call p95 | No repeatable regression greater than 5% after accounting for noise |
| Long-input transient memory | Target at least 20% reduction in measured peak live heap on token-heavy inputs; no unexplained regression elsewhere |
| Cold initialization | Measured improvement; show native and Wasm separately |
| Wasm raw/compressed bytes | No increase over 5% without an explicit documented tradeoff |
| Clean compile time/compiler peak memory | No increase over 10% without a demonstrated runtime/maintenance benefit |

If a gate fails, simplify or revise the responsible change. Any accepted exception must identify the affected workload/platform, benefit, measured cost, and rationale; do not silently relax the baseline or omit slow cases.

### Documentation and release

Update `ARCHITECTURE.md` for actual data flow, allocation/ownership, immutable lookup, and generated-data authoring. Update `AGENTS.md` and relevant package/build documentation for real toolchain changes. Update `docs/release.md` only if CI/release setup changes. Keep prior exploration documents as historical assessments.

Add an accurate changelog entry when the implementation is ready. Do not bump versions during planning; use the existing version tool for an authorized release. A C++ source-API correction or raised consumer toolchain floor must be called out explicitly. Publishing is outside this plan's execution scope.

## 14. Review structure and completion checklist

Suggested review boundaries:

1. Characterization/measurement infrastructure and ownership fix, with any API decision recorded.
2. C++23 toolchain adoption and platform support documentation.
3. Small allocation reductions.
4. Immutable script descriptors and generation checks.
5. Flat tokenizer/prefix lookup structures.
6. Custom-handling extraction.
7. Token-buffer memory reduction and individually justified tuning.
8. Integration evidence, rebuilt artifacts, final documentation.

Keep each boundary reversible while later changes are developed. Record generation-format changes alongside generated output; roll back generator and data together. Do not ship duplicate old/new engines or permanent test-only switches.

## 15. Implementation tracking

Updated 2026-09-29 during implementation. The following work is present in the current working tree; this is not an acceptance sign-off.

| Package | Current state | Evidence / remaining work |
|---|---|---|
| P0 | Benchmark and fixture checks added | `make test` passes with shared fixture loading. `native/bench/benchmark.cpp` records medians and the 95th percentile of 31 sample-average timings for seven workloads at short, 4 KiB, and 1 MiB sizes. `native/bench/short_call.cpp` separately measures 10,000 warmed calls individually to report actual per-call p50/p95 latency. `native/bench/memory.cpp` with Valgrind Massif records peak heap on six 1 MiB workload modes. `make bench-alloc-linux` builds and runs the checked-in glibc `LD_PRELOAD` probe; it wraps `malloc`/`calloc`/`realloc`/`free` during conversion, including the output builder, and reports live/peak requested bytes for tracked allocations. Set `ALLOC_BENCH_WARMUPS=1` to exclude one-time script-map construction from the reported event counts. Probe builds remain isolated from production builds. Memcheck/XTree cannot start because Valgrind cannot redirect the stripped loader's `memcmp`. The browser fixture page reports machine-readable status and checks repeated protected-span calls; broader script-pair coverage remains. |
| P1 | Implemented | `TranslitBuffer` consistently frees `realloc` storage; Dart releases copied output in `finally`; repeated C ABI release and builder tests pass. The authorized C++ owner signature change is documented. |
| P2 | Implemented in configuration; hosted platform validation incomplete | The engine target requests C++23 via native CMake; Flutter plugin native build entry points require CMake 3.20+ (Android pins 3.22.1), Apple podspec/Xcode settings and both Wasm build scripts select C++23. Flutter example runner helper targets retain upstream C++14/C++17 settings; they do not compile the engine, which is its own C++23 CMake target. The native CMake target requires C++23 and publishes `cxx_std_23` to consumers. CMake 3.20.5 and 4.4.3 configure/build/CTest pass locally. Linux CMake/CTest, MinGW GCC 16.2, Android arm64 (CMake 3.22.1, NDK 28.2.13676358 / Clang 19.0.1, API 21), and Emscripten 4.0.3 builds pass. Windows/MSVC, macOS/Apple Clang, and Flutter Apple/Windows plugin jobs still need hosted runners. Both Wasm debug entry points were checked earlier; PowerShell syntax/runtime remains unexecuted because PowerShell is unavailable. |
| P3 | Implemented | Reader/writer objects use automatic storage; Tamil substitution uses a fixed array; transparent script-cache lookup avoids a temporary string. Trie insertion now decodes UTF-8 spellings directly instead of allocating a temporary UTF-32 string. A cold Devanagari→Telugu call used 447 C++ `new`/`new[]` allocations / 15,371 B versus 466 / 15,967 B with the prior insertion path (19 fewer allocations, 596 B fewer). Warm counts were unchanged at 2 / 163 B. A global C++ allocation counter was used for this focused comparison; it does not count the output builder's direct `malloc`. Three CPU-pinned 31-sample comparisons against the old insertion path gave per-workload median timing ratios of 1.005–1.030 across 1 MiB cases with matching checksums; no repeatable regression exceeded 3%. Current short-call p50/p95 is measured per call in a separate probe (see P8 evidence). |
| P4 | Implemented | Generated immutable script descriptors, indexed writer views, and compile-time bounds/uniqueness checks replace runtime script metadata parsing. Identical category/equivalent lists share index ranges. Two regenerations were byte-identical. A negative compile probe that changed a generated script-name offset to 65,535 failed at `validScriptDescriptors`, confirming invalid offsets are rejected at compile time. |
| P5 | Immutable tokenizer implemented; throughput and Wasm-size gates open | Ranked root bitmaps with exact root-map and child-sequence interning, sorted non-root edges, 4-byte `ReaderNode`, 6-byte `ReaderData`, BMP-bound generator. The 1,048,971-case spelling/options/output differential and 19,200 uppercase ISO/IAST folding cases ran before the final root interning; native tests and fresh benchmark hashes match after it. Three fresh CPU-pinned pairs against a newly compiled `d8b7140` baseline show 1 MiB changes of -1.4% Devanagari→Telugu, -9.3% Tamil, +1.9% Latin input, +4.6% virtual Indic, +2.0% expansion-heavy, -2.3% protected spans, and +1.2% mixed protected spans. Virtual Indic misses the 3% gate. Six pairs versus the exact-root-only predecessor kept all long-input medians within 3%; ten short-call pairs kept p95 within -15.90% to +1.57%. Warm requests are 3–5 versus 9–11 baseline; peak heap falls about 30% for Indic workloads. Current standalone Wasm is 57,512 B / 28,617 B gzip and JS/Wasm is 91,715 B / 42,848 B gzip, up 17.7% and 13.9% compressed; data section is 36,965 B vs 17,490 B baseline. Native shared library is 95,128 B vs 135,376 B (-29.7%). P5 is not merge-ready.
| P6 | Implemented | Anuswara selection is a direct helper; Tamil consonant context rules now live in `InputReader::applyTamilConsonantContext`. Existing shared Tamil cases pass with byte-identical expected output. Gurmukhi expansion and target-specific encoding remain beside their respective reader/writer paths. |
| P7 | Implemented; Linux allocator-event profile measured | `InputReader` reserves about two-thirds of input bytes for Indic/Tamil and the full byte count for Latin/other sources. This cheap capacity estimate adds no prepass; the reader remains authoritative and vector growth covers atypical inputs. Three CPU-pinned 31-sample runs of each build against a fresh C++23 `HEAD` build have matching checksums; medians of the runs show no workload regression above 3%. Virtual Indic had one paired +5.8% result, while the other two pairs were +2.9% and -0.5%, so the hit did not repeat. Fresh Valgrind Massif runs show peak-live-heap reductions of 28.8–29.9% for Indic, expansion-heavy, protected, mixed, and virtual Indic inputs; Latin is 0.5% lower. The checked-in Linux `LD_PRELOAD` probe counts malloc/calloc/realloc/free events around conversion, including the output builder. Cold Indic-family calls use 40.8–83.8% fewer malloc/realloc calls and 28.0–31.8% fewer requested bytes (malloc and realloc request bytes summed); Latin requests fall by 79.3% in call count and 0.6% in requested bytes. With one warmup conversion, the current implementation records 3–5 allocation requests per measured conversion across all six workloads, versus 9–11 on the baseline. Checksums match. The probe relies on glibc allocator symbols, so these event counts are Linux-specific rather than a cross-platform measurement. |
| P8 | Partial; local validation broad, hosted/runtime checks open | On the current exact-child-sequence-interned source, `make test`, CMake Release/CTest, ASan/UBSan (`ASAN_OPTIONS=detect_leaks=0`), direct Dart analysis, release verification, deterministic regeneration of all three generated headers, Node Wasm smoke conversions, and three-pair native benchmarks passed. The 1,048,971-case output differential predates the final exact-root interning; native fixtures and benchmark output hashes match after it. Browser fixtures could not run because the sandbox denied starting a local HTTP server. Flutter tests were not rerun because the Flutter launcher cannot write its SDK cache. `yarn test` remains blocked by ts-jest incompatibility with installed TypeScript 7.0.2. LeakSanitizer, Android runtime, Windows execution, MSVC, Apple Clang/SDK, and hosted plugin jobs remain unverified.

The CI workflow now also builds the Flutter example on `macos-14` and `windows-2022`. These jobs exercise the CocoaPods and Windows CMake plugin integration paths with the configured C++23 mode; their hosted results remain pending because this worktree has not been pushed.

The clean optimized compile comparison recorded before the reserve-4 follow-up used Clang 22.1.8: the implementation took 3.49 s / 217,424 KiB peak RSS versus 3.94 s / 209,868 KiB for a fresh C++23 build of `HEAD` (+3.6% compiler memory, 11.3% faster). For the final reserve-4 source, three alternating clean C++23 `-O3 -DNDEBUG` compilations of `inditrans.cpp` alone took a median 3.908 s / 222,440 KiB peak RSS versus 4.185 s / 207,512 KiB for an archived `HEAD` copy (6.6% faster, 7.2% more compiler memory). Elapsed time used `steady_clock`; peak RSS was sampled from `/proc/<pid>/status` every 5 ms. This is translation-unit evidence, not a whole-target build-time measurement. ASan/UBSan/LeakSanitizer results on the current reserve-4 revision are listed in the P8 row above.

After non-root suffix sharing, three alternating clean Clang 22.1.8 C++23 `-O3 -DNDEBUG` translation-unit compilations took a median 2.888 s / 209,576 KiB peak RSS versus 3.490 s / 207,768 KiB for archived `HEAD` (-17.2% compile time, +0.9% compiler memory). The source compiled was the current generated-table implementation; RSS was sampled from `/proc/<pid>/status` every 5 ms.

#### P5 reader-lookup experiments

Each row is a separate CPU-pinned sequential comparison. Values are median milliseconds for 1 MiB cases, listed in this order: Devanagari→Telugu, Tamil output, Latin input, virtual Indic input, protected spans. The first seven prototypes use a fresh C++23 `HEAD` baseline and passed their recorded correctness checks. The node-local perfect-hash row compares three runs of the current worktree with and without finalization; both variants had matching checksums, and the first hash variant passed the full native suite. Each prototype was removed after missing the throughput or footprint gate.

| Prototype | Baseline → candidate medians (ms) | Result |
|---|---|---|
| Native first-scalar candidate arrays | 13.83→18.05, 17.30→20.59, 29.34→54.74, 12.37→17.45, 4.94→7.27 | Added ~136 KiB; 19–87% slower |
| PMR pool-backed trie | 13.65→15.47, 17.20→17.96, 29.22→32.14, 12.65→14.51, 4.95→7.34 | 4–48% slower |
| Runtime flattening with binary search | 13.67→22.31, 17.09→24.89, 29.02→46.43, 12.29→25.57, 5.03→7.01 | 39–108% slower |
| Runtime flattening with open addressing | 13.54→15.67, 17.47→18.11, 29.33→40.38, 12.32→15.87, 4.90→7.09 | 4–45% slower |
| Direct compare for small nodes, hash slots for larger nodes | 13.82→16.48, 17.18→19.44, 29.43→38.36, 12.47→17.09, 5.02→6.37 | 13–37% slower |
| Inline single-child nodes, root hash map retained | 13.65→15.54, 17.42→18.04, 29.07→32.84, 12.27→14.52, 4.89→7.18 | 4–47% slower |
| Slab arena with current hash-map lookups | 13.86→15.75, 17.29→18.11, 29.30→33.31, 12.59→14.48, 5.03→6.56 | 5–30% slower |
| Runtime node-local perfect hash tables, 2× power-of-two slots | 12.82→13.94, 16.26→16.87, 26.62→30.07, 12.22→13.77, 4.70→4.80 | 2–13% slower across the five original comparison workloads; checksums match |
| Compact scalar-leaf table with bitmap/rank lookup | 14.70→17.24, 18.39→22.89, 30.75→32.69, 13.66→17.63, 13.76→14.85, 5.29→5.51, 8.67→10.04 | 4–29% slower across the seven workloads; checksums match; removed |
| Dense 256-entry scalar pages | 14.70→16.79, 18.39→19.88, 30.75→33.20, 13.66→16.82, 13.76→14.68, 5.29→5.40, 8.67→9.56 | 2–23% slower across the seven workloads; checksums match; removed |
| Runtime flattened trie with chained hash buckets | 14.88→15.29, 18.76→18.69, 31.65→31.57, 14.14→14.96, 13.86→14.55, 5.35→5.57, 8.91→9.24 | Two pinned pairs; checksums match. Median-of-pairs regressions: virtual Indic +5.8%, expansion-heavy +4.9%, protected +4.1%, mixed protected +3.7%; removed |

Two cheaper node-local hash variants were also tried in separate pinned pairs. A single 64-bit multiply/shift hash regressed all seven current workloads by 3–10%. A shift/XOR hash was close or faster for Tamil, Latin, expansion-heavy, protected, and mixed-protected input, but regressed Devanagari→Telugu by 9.8% and virtual Indic by 6.8%; it fails the repeatable 3% throughput gate. The perfect-hash experiment passed all native tests, but its seed-search construction and warmed lookup still did not meet the gate. It was removed without spending time on Wasm generation or footprint optimization.

A final scalar-leaf prototype stored safe single-codepoint terminals in per-reader sparse page bitmaps with rank-indexed token values and removed those leaves from the heap trie. One CPU-pinned 31-sample run against a trie-only build produced the seven-workload medians in the last table row, in benchmark order: Devanagari→Telugu, Tamil output, Latin input, virtual Indic input, expansion-heavy, protected spans, and mixed protected spans. All checksums matched, but every workload regressed. Both the flat binary-search and bitmap/rank forms were removed. This narrows the remaining search: a scalar fast path that adds a lookup step to the current root hash is not a suitable replacement by itself.

A dense 256-entry page table was then tried to remove the bitmap rank operation. Its first CPU-pinned 31-sample comparison matched all checksums and regressed every workload by 2–23%. A second pinned comparison showed +13.4%, +7.6%, +5.1%, +23.3%, +4.8%, -2.2%, and +10.8% in the same seven-workload order. The protected-span result changed direction, while the other six remained slower, so the representation was removed. Both scalar rows list seven workloads in benchmark execution order; the earlier rows list the five comparison workloads defined above.

A runtime flattening prototype that retained hash semantics but replaced per-node open-addressing tables with flat bucket arrays and collision chains was measured in two CPU-pinned 31-sample pairs against the current trie. Checksums matched. The median-of-pairs showed repeatable regressions on virtual Indic (+5.8%) and expansion-heavy (+4.9%) inputs, missing the gate; protected and mixed-protected inputs also averaged +4.1% and +3.7%. It was removed without running the full fixture suite or measuring retained heap because it failed the throughput gate.

A generated immutable flat-trie prototype was also tested with sparse root-page bitmaps, rank lookup, and linear scans for nodes with at most four edges. Native CTest passed. A CPU-pinned benchmark run matched all seven checksums, but the 1 MiB medians were 35.68, 42.40, 84.15, 35.57, 28.91, 11.11, and 20.53 ms, compared with the retained runtime-trie medians of 13.57, 17.13, 28.77, 12.76, 12.78, 5.11, and 8.03 ms. This is a diagnostic comparison rather than a paired acceptance run; the regressions are large enough to reject this representation without more tuning. The candidate and its generator have been removed. The selected P5 exception retains the measured cached runtime trie with its small-map reserve.

After restoring the cached runtime trie, three CPU-pinned 31-sample comparisons against a fresh `HEAD` build with Clang 22.1.8 and `-O3 -DNDEBUG` matched all checksums. Median-of-run 1 MiB changes for Devanagari→Telugu, Tamil output, Latin input, virtual Indic input, expansion-heavy input, protected spans, and mixed protected spans were -3.6%, -2.5%, +0.7%, -0.4%, -6.1%, -2.1%, and -3.6%, respectively. No repeatable throughput regression exceeded the 3% gate.

Three additional CPU-pinned short-call pairs measured 10,000 individual warmed calls per workload; checksums matched. Median-of-run p95 changes in the same workload order were -12.3%, -10.4%, -1.3%, -13.2%, -49.0%, -28.4%, and -24.1%, all within the 5% short-call regression gate.

A reserve of 8 buckets was first selected for nested maps. Three pinned 31-sample comparisons against the same trie without this reserve produced matching checksums; 1 MiB median throughput changed by -5.8% to +2.4% across seven workloads, with no regression above 3%. The 4 KiB virtual-Indic case improved 5.4%; short-call p95 changed by -8.2% to +4.2%, below the 5% gate. On cold 97-byte calls, requested live heap fell from 13,384 B to 13,096 B for Devanagari and from 139,864 B to 134,328 B for virtual Indic; allocator request counts stayed at 448 and 3,967. A follow-up reduced the reserve to 4 buckets; the isolated evidence appears below.

The reserve-4 follow-up was compared with reserve 8 using Clang 22.1.8, `-O3 -DNDEBUG`, and the same source otherwise. Three CPU-pinned 31-sample long-input process pairs matched all seven checksums; median-of-run changes ranged from 0.1% faster to 2.2% slower, within the 3% gate. Eleven per-call short-call process runs with order variation showed p95 medians from 0.7% to 6.9% faster. Cold small-input probes reduced retained requested bytes from 13,096 to 12,232 B for Devanagari and 134,328 to 117,960 B for virtual Indic; Latin fell from 22,824 to 17,960 B. The virtual-Indic allocation request count rose from 3,967 to 3,972; Devanagari and Latin counts were unchanged. This remains a cold initialization optimization, not a replacement for P5's zero-construction target.

The allocator probe separates lazy table initialization from steady-state conversion by accepting a warmup count. For the current implementation, cold conversion made 448–3,972 malloc/realloc calls across these six workloads; after one conversion warms the script caches, the measured conversion uses 3–5 such calls. The baseline warmed path uses 9–11. Checksums match. This confirms that the remaining tokenizer trie allocations are first-use initialization work; they do not recur on steady-state conversions. The static lookup prototypes above were timed on warmed conversions, so their reported regressions are lookup costs rather than first-use construction costs.

**Historical P5 status, before the ranked bitmap roots.** Earlier hashed-root and sorted-root static candidates replaced runtime trie construction with immutable generated tables. The hashed-root candidate passed the recorded native, Flutter, Node, browser fixture tests, 49,152-case spelling/option differential, and cold allocation probes, but paired long-input runs and rebuilt Wasm artifacts missed the plan's throughput and size gates. Those checks do not establish correctness of the current ranked-bitmap source.

A follow-up changed the linear-scan cutoff from 8 to 9, affecting two non-root nodes with degree 9 while leaving the degree-17 node on binary search. Two alternating CPU-pinned pairs against cutoff 8 showed mixed results: Latin input regressed 5.3–6.8%; Tamil changed by +0.6–2.4%; the other workloads varied from 11.3% faster to 2.1% slower between pairs. Cutoff 9 was rejected as noisy and not a consistent improvement; production retains cutoff 8. The static-table P5 performance and Wasm-size gates remain open.

A generated 256-entry ASCII root page was also measured for Latin readers. It improved Latin input by roughly 7% against the same flat-table build, but repeated comparisons showed regressions on virtual Indic and other workloads. That path was removed. The later sorted-root representation also removed root hashing; root hashes are not present in current production source.

**Current P5 status, 2026-09-29.** With pending-match caching, protected-span `string_view::find`, direct second-page checks, and exact root-map interning, the ranked-root bitmap candidate meets the measured throughput gates against `d8b7140`: three paired 1 MiB medians range from 9.78% faster to 0.48% slower; 4 KiB medians range from 6.51% faster to 1.86% slower; ten short-call p95 medians improve 9.29–52.00%. Hashes match. Same-flags Wasm gzip remains +18.9% standalone and +15.6% JS/Wasm, above the +5% limit. Exact root interning removes 14 root pages and 392 child refs with no runtime code changes; three paired runs versus the prior table layout stay within ±1.3% long-input timing. The packed 4-byte node saves 1,096 B in the Wasm data section versus the prior node layout; current `ReaderData` is 6 bytes and stores root page count explicitly. Differential coverage includes 1,048,971 generated-spelling/options/output cases and 19,200 uppercase ISO/IAST cases before the root interning change; current native tests and benchmark hashes cover that final representation. Cross-script terminal-token deduplication, root-child sequence sorting, and non-root edge-block sorting were tested and reverted after performance or gzip-size regressions. The current build has meaningful allocation, peak-heap, native-size, and throughput advantages, but P5 remains not merge-ready because compressed Wasm size is over target.

Cold profiles use one measured conversion with no warmup and a nominal 32-byte input target; seed repetition makes the actual inputs 36–118 bytes. The probe reported no tracking-table overflow in these cases.

| Mode | Actual input → output bytes | malloc/realloc requests | Requested bytes still live |
|---|---:|---:|---:|
| Devanagari → Telugu | 97 → 97 | 448 | 12,232 B |
| Latin → Devanagari | 43 → 90 | 625 | 17,960 B |
| Expansion-heavy Devanagari | 36 → 72 | 449 | 12,232 B |
| Protected spans | 71 → 67 | 448 | 12,232 B |
| Mixed protected spans | 118 → 114 | 448 | 12,232 B |
| Virtual Indic → ISO | 97 → 49 | 3,972 | 117,960 B |

Before the nested-map `reserve(8)` tuning, the focused 97-byte Devanagari profile measured 448 malloc requests / 16,964 requested bytes and a 15,042-byte peak with no warmup. After one warmup, the measured conversion made 3 malloc requests / 1,756 requested bytes, left zero additional tracked bytes live, and peaked at 1,658 bytes. Checksums and output lengths matched. Requested live/peak sizes exclude allocator overhead and memory allocated before the counter reset.

Before the nested-map `reserve(8)` tuning, a 97-byte virtual-Indic input with a 49-byte output measured 3,967 malloc requests / 158,652 requested bytes and a 143,184-byte peak. After one warmup, it made 3 requests / 1,708 requested bytes, left zero additional tracked bytes live, and peaked at 1,658 bytes. Checksums and output lengths matched.

The allocator probe was rerun against the restored runtime-trie source. Cold Devanagari input made 448 malloc requests / 16,676 requested bytes, with 13,096 bytes live and a 14,754-byte peak; after one warmup, the conversion made 3 requests / 1,756 bytes, with zero additional live bytes and a 1,658-byte peak. Cold virtual Indic made 3,967 malloc requests / 153,116 requested bytes, with 134,328 bytes live and a 137,904-byte peak; after one warmup, it made 3 requests / 1,708 bytes, with zero additional live bytes and a 1,658-byte peak. Both runs used 97-byte inputs, matching output lengths and checksums, and the probe reported no untracked allocations.

Three CPU-pinned 31-sample runs of the current implementation and a fresh C++23 build from `HEAD` used the same Clang 22.1.8 compiler, `-O3 -DNDEBUG`, a fixed CPU, and the seven 1 MiB workloads. All checksums matched. Median-of-run changes are within 3% for every workload:

| Workload | Baseline median | Current median | Change |
|---|---:|---:|---:|
| Devanagari to Telugu | 13.70 ms | 13.57 ms | -0.9% |
| Tamil output | 17.41 ms | 17.13 ms | -1.6% |
| Latin input | 29.39 ms | 28.77 ms | -2.1% |
| Virtual Indic input | 12.56 ms | 12.76 ms | +1.6% |
| Expansion-heavy | 13.31 ms | 12.78 ms | -4.0% |
| Protected spans | 5.09 ms | 5.11 ms | +0.4% |
| Mixed protected spans | 8.28 ms | 8.03 ms | -3.0% |

Virtual Indic had one paired +5.8% result; its other pairs were +2.9% and -0.5%. The earlier benchmark's `p95` was a percentile of sample-average timings rather than individual call latency; a separate warmed per-call probe below measures short-call p50/p95 directly. A packed generated reader-lookup experiment was reverted after it exceeded the Wasm size limit and its lookup path regressed throughput. See the P5 table for native and runtime trie experiments.

Three CPU-pinned runs of the warmed per-call probe used the same Clang 22.1.8 compiler, `-O3 -DNDEBUG`, fixed CPU, and 10,000 individually timed conversions per workload. The baseline is the repository `HEAD` source compiled in C++23 mode; current uses the implementation worktree. Every workload's checksums match across all six runs. Values below are medians across the three process runs; the proposed 5% short-call p95 regression gate passes for all workloads.

| Workload | p50 baseline → current | p50 change | p95 baseline → current | p95 change |
|---|---:|---:|---:|---:|
| Devanagari to Telugu | 1,393 → 1,193 ns | -14.4% | 1,443 → 1,232 ns | -14.6% |
| Tamil output | 1,703 → 1,483 ns | -12.9% | 1,764 → 1,502 ns | -14.9% |
| Latin input | 1,342 → 1,182 ns | -11.9% | 1,383 → 1,212 ns | -12.4% |
| Virtual Indic input | 1,313 → 1,192 ns | -9.2% | 1,352 → 1,222 ns | -9.6% |
| Expansion-heavy | 451 → 230 ns | -49.0% | 471 → 231 ns | -51.0% |
| Protected spans | 621 → 411 ns | -33.8% | 631 → 421 ns | -33.3% |
| Mixed protected spans | 1,142 → 972 ns | -14.9% | 1,162 → 983 ns | -15.4% |

The first non-LTO output measured 60,728 B standalone versus the 58,266 B baseline (+4.2% raw, +10.2% gzip); JavaScript/Wasm was 94,156 B versus 90,780 (+3.7% raw, +6.7% gzip). Gzip used `gzip -n -9`. This exceeded the proposed 5% compressed-size gate. A later offset-only metadata representation reduced the non-LTO outputs to 59,401 B / 28,603 B gzip and 92,388 B / 41,599 B gzip, still just above the compressed limit.

A follow-up layout trial stored generated string offsets and lengths in separate arrays to remove structure padding. Native fixtures passed. The standalone Wasm fell to 57,849 B raw but grew to 29,292 B compressed versus the 58,266 B / 26,101 B baseline; the JavaScript/Wasm bundle fell to 90,304 B raw and 41,529 B compressed versus 90,780 B / 39,554 B. The standalone compressed increase was 12.2%, so the layout was reverted and the production artifacts were rebuilt from the retained representation.

A later metadata-only layout trial keeps `StringRef` (offset plus length) for hot character lookup and `ScriptInfo` names, while storing NUL-terminated script-list keys and values as offsets only. Native tests and generated-data validation pass. The refreshed artifacts are 59,401 B / 28,603 B compressed for standalone Wasm and 92,388 B / 41,599 B for the JavaScript/Wasm bundle. Against the same EMSDK baseline, this intermediate output was +1.9% raw / +9.6% compressed and +1.8% raw / +5.2% compressed, respectively. Five CPU-pinned paired runs of the current short-call executable and a fresh C++23 `HEAD` executable produced matching checksums; current p95 medians were 5.7–47.1% lower across the seven workloads. The subsequent LTO release layout met the size gate against the previous release artifacts; see the final artifact measurements below.

A second trial interleaved each reference's 16-bit offset and 8-bit length in an explicit three-byte array. Native fixtures passed, and raw artifacts fell to 57,819 B standalone / 90,280 B JavaScript, but compressed sizes rose to 30,006 B / 40,801 B; standalone was 15.0% above the baseline. Three pinned short-call runs also showed 14.7–33.9% warmed p95 regressions against the retained representation. The encoding and artifacts were reverted.

An Emscripten LTO trial (`-flto`) with matching `-Oz` flags compared the original `HEAD` source and the implementation source under the same flags. Standalone Wasm was 54,353 B / 24,312 B gzip for `HEAD` and 55,575 B / 26,241 B for the earlier implementation layout (+2.3% raw, +7.9% compressed), so that isolated same-flags comparison missed the 5% compressed threshold.

A follow-up used the LTO release build with the offset-only metadata layout and nested-map reserve. The final checked-in artifacts are 54,132 B / 26,028 B gzip standalone and 86,671 B / 38,641 B gzip JavaScript/Wasm. Compared with the previous non-LTO baseline artifacts (58,266 B / 26,101 B and 90,780 B / 39,554 B), raw size falls 7.1% and 4.5%, while compressed size falls 0.3% and 2.3%. Compared with a non-LTO build of the same current source, LTO reduces JavaScript/Wasm raw/gzip size by 6.2%/7.3%. Three CPU-pinned Node runs of the same current source with and without LTO produced identical checksums; LTO p95 ranged from 21.1% faster to 1.7% slower across the seven short-call workloads, within the 5% gate. The final LTO standalone Wasm passed all 47 shared fixture conversions and 100 repeated protected-span calls; `yarn test` passed 47/47 with the LTO JavaScript/Wasm bundle. Headless Chromium loaded the final LTO browser bundle and passed all 47 shared fixture conversions plus 100 repeated protected-span calls (48/48). For reproduction, serve the repository root with `python3 -m http.server 8765 --bind 127.0.0.1`, then open `/js/tests/test.html` with headless Chromium using `--no-sandbox --disable-crash-reporter --disable-breakpad --disable-dev-shm-usage --disable-gpu --disable-extensions --no-zygote --disable-features=NetworkServiceSandbox --virtual-time-budget=30000 --dump-dom`; the test page reports `data-status="passed"`, `data-passed="48"`, and `data-failed="0"`. The release shell script was rebuilt and byte-matched the measured LTO outputs. The PowerShell script mirrors the command but remains unexecuted locally. The shipped artifacts meet the size gate against the prior release artifacts; the same-LTO source comparison remains recorded to show the source-layout cost.

After removing the slower static reader table, both checked-in Wasm artifacts were rebuilt from the retained runtime-trie source. The current files are 54,342 B / 26,086 B gzip standalone and 87,055 B / 38,876 B gzip JavaScript/Wasm. Against the prior release artifacts above, raw size changes are -6.7% and -4.1%, and gzip size changes are -0.1% and -1.7%. `yarn test` passed all 47 tests against the rebuilt JavaScript/Wasm bundle. Headless Chromium also passed the current browser page with all 47 shared fixture conversions and 100 repeated protected-span calls (48/48, `data-status="passed"`, `data-failed="0"`). The first server attempt was denied by the sandbox; rerunning with local socket access succeeded.

#### Isolated `InputReader::tokenUnitCapacity` comparison

To isolate the type-based reserve from the other changes, a temporary build replaced only the Indic/Tamil capacity estimate with the former `input.size()` reserve. Three CPU-pinned process pairs used Clang 22.1.8, `-O3 -DNDEBUG`, and the same source otherwise. All seven 1 MiB workload checksums matched; median-of-run throughput changed by -3.1% to +1.4% for the smaller reserve, with no repeatable regression above the 3% gate. Twenty per-call short-call process pairs (10,000 calls per workload, with run order alternated for half the pairs) also had matching checksums. Across process-run medians, the smaller reserve's p95 changed from -6.6% to +3.4%; no workload exceeded the 5% regression gate. These short-call timings were noisy and do not support claiming a speedup.

The Linux allocator probe isolates the memory effect on cold 1 MiB conversions. Compared with reserving one `TokenUnit` per byte, the current estimate lowered peak simultaneously tracked requested bytes by 32.0% for Devanagari and 31.8% for virtual Indic. Latin retained the full-byte reserve and was unchanged. Output lengths and checksums matched.

A further capacity trial changed only the Indic/Tamil estimate from two-thirds to one-third of input bytes. Three CPU-pinned Clang 22.1.8 `-O3 -DNDEBUG` benchmark runs per build used the same 1 MiB cases and matched all checksums. Median-of-run throughput was effectively unchanged for expansion-heavy (-0.5%), Latin input (-0.8%), protected spans (+0.3%), and mixed protected spans (+0.8%), but regressed 11.1% for Indic→Indic, 7.9% for Tamil output, and 11.5% for virtual Indic. A cold allocator probe on Devanagari and virtual Indic showed one additional allocation and free, 8.39 MB more total requested bytes, and 7.34 MB more peak live bytes with the lower estimate. The lower estimate was discarded: vector growth on token-dense Indic inputs both misses the throughput gate and raises peak memory.

A temporary `InputTokenBuffer` prototype used 80 aligned inline `TokenOrString` slots with the current vector as an overflow path. Five CPU-pinned process runs per build measured 10,000 warmed short calls per workload with identical checksums. Median process-run p95 latency changed by +12.2%, +7.1%, +3.7%, 0.0%, -3.7%, +6.8%, and +6.0% in benchmark order (Indic→Indic, Tamil output, Latin input, virtual Indic, expansion-heavy, protected spans, mixed protected spans). Four workloads exceeded the 5% gate. A 128-slot variant was slower still on most workloads. Both prototypes were removed; the stack buffer’s latency and added code did not justify the allocation tradeoff.

A 1 MiB unmatched raw UTF-8 workload (`漢字🙂` repeated, Devanagari reader) exposes a limitation of the selected reserve: the warmed allocator probe's peak tracked requested bytes were 17,825,877 B, mostly the oversized token vector. A temporary first-character check capped the initial reserve at 16 entries when no token began at the input start; output checksums matched and peak tracked requested bytes fell to 2,097,162 B. That variant did not meet the broader gates: three pinned long-input pairs regressed 3.21% on Indic→Indic, 3.11% on protected spans, 4.98% on Tamil output, and 4.52% on virtual Indic, although the raw UTF-8 case improved 0.96%. Ten short-call process pairs kept every p95 within 5%, but a follow-up thresholded form that left short inputs on the old capacity path still regressed short-call p95 by 6.9–11.4% on four workloads. The content-aware reserve prototype was removed; raw unmatched text remains a memory limitation of the type-based estimate.

| Workload | Smaller reserve total / peak requested bytes | Full-byte reserve total / peak requested bytes | Peak reduction |
|---|---:|---:|---:|
| Devanagari → Telugu | 18,890,944 / 17,840,452 B | 27,280,264 / 26,229,772 B | 32.0% |
| Virtual Indic → ISO | 18,508,504 / 17,961,684 B | 26,897,824 / 26,351,004 B | 31.8% |
| Latin → Devanagari | 28,432,108 / 29,382,356 B | 28,432,108 / 29,382,356 B | 0% |

Package dry-runs were checked after the artifact rebuild. `npm pack --dry-run` passed with `npm_config_cache` directed to `/tmp`; its prepack step passed all 47 Node tests and the package included the rebuilt 87,055-byte JavaScript/Wasm bundle. It passed again after the reserve-4 update. `dart tool/verify_release.dart --check-packages --check-artifacts` generated and listed the Flutter package contents, then failed its pub.dev lookup because network access is restricted. A direct `dart pub publish --dry-run` attempt was rejected by automatic review because it may transmit package contents to pub.dev; do not retry without user approval. Pub exposes no offline publish-validation mode; `--skip-validation` bypasses validation rather than verifying it. The verifier's npm subcheck also hit the read-only default npm cache; the standalone dry-run passed with a writable cache.

After the suffix-shared P5 rebuild, `npm pack --dry-run` passed again when both npm and Emscripten caches were redirected to `/tmp`. Its prepack step passed all 48 Node tests and packaged the current 126,231-byte JavaScript/Wasm bundle. The first retry without a writable `EM_CACHE` failed because the EMSDK cache directory is read-only.

Cold-process peak live heap uses Valgrind 3.25.1 Massif (`--stacks=no --time-unit=B`) on a 1,048,576-byte target repeated seed. The baseline is a fresh C++23 optimized build of `HEAD`; both executables use the same `native/bench/memory.cpp` and Clang 22.1.8 with `-O3 -DNDEBUG`. The checked-in glibc `LD_PRELOAD` wrapper measures malloc/calloc/realloc/free events around conversion; malloc and realloc requested bytes are summed. Input construction occurs before enabling the probe; the returned std::string is alive at the report point. Checksums match in every row.

| Input mode | Peak heap baseline → current | Peak reduction | malloc/realloc calls baseline → current | Requested bytes baseline → current |
|---|---:|---:|---:|---:|
| Devanagari to Telugu | 28,555,584 → 20,018,552 B | 29.9% | 2,762 → 448 | 27,478,638 → 18,890,080 B |
| Latin to Devanagari | 31,717,488 → 31,565,112 B | 0.5% | 3,022 → 625 | 34,925,220 → 34,720,054 B |
| Expansion-heavy Devanagari | 29,604,096 → 21,067,064 B | 28.8% | 2,763 → 449 | 30,622,237 → 22,034,324 B |
| Protected/raw spans | 28,555,520 → 20,018,488 B | 29.9% | 2,762 → 448 | 27,417,794 → 18,829,764 B |
| Mixed protected spans | 28,555,584 → 20,018,552 B | 29.9% | 2,762 → 448 | 27,443,064 → 18,854,506 B |
| Virtual Indic to ISO | 28,727,320 → 20,169,112 B | 29.8% | 6,712 → 3,972 | 27,113,706 → 18,492,336 B |

Massif and the external glibc allocation probe both run in this environment. Valgrind Memcheck/XTree cannot start because the stripped dynamic loader does not export the `memcmp` symbol Valgrind requires for redirection; the preload probe supplies allocator event counts but does not replace a full memory-safety or leak check. Earlier XTree results from the exact protected-span pre-count policy are not current and were removed from this comparison.

Completion requires:

- [ ] C++23 selected and verified on every supported build path.
- [ ] Toolchain/deployment support floor documented and tested.
- [x] Ownership contract corrected, including the explicit C++ overload decision.
- [x] Immutable built-in script data and lookup structures used in production. Script descriptors, Tamil prefix lookup, and built-in tokenizer lookup use generated immutable tables; P5's performance/size acceptance remains open.
- [x] Compile-time structural checks fail on invalid generated data.
- [x] Per-call reader/writer and substitution allocations removed; peak live heap reduced on the measured Indic workloads (P3/P7 evidence above).
- [x] Custom deterministic handling separated without behavior drift.
- [ ] Cross-platform correctness, sanitizer, and package checks recorded. Linux, Flutter native tests, Node/WASM, browser, MinGW and Android builds have local evidence; remote Apple/MSVC/plugin builds, Android runtime, and LeakSanitizer remain open.
- [ ] Before/after speed, allocation, memory, binary-size, and compile-cost results satisfy the gates or have accepted exceptions. Existing P3/P7 evidence remains valid for those source revisions; the current tokenizer meets measured throughput gates but still fails compressed Wasm size by more than the +5% limit.
- [x] Public C/Dart/TypeScript APIs preserved; the approved C++ source compatibility change is documented.
- [x] Existing coding style preserved; no unused framework, unrelated dependency, or speculative optimization added.
- [x] Both exploration documents remain separate from this implementation plan.

The checklist above remains the completion criterion. Do not mark the implementation complete until the open package work and proposed gates have evidence or an explicitly accepted exception.
