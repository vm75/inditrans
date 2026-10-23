# C++23 engine security audit

Audit date: 2026-10-22  
Audited base: `fdaf6b5` (`v0.14.0`)  
Scope: GitHub issue [#61](https://github.com/vm75/inditrans/issues/61)

This focused source and dynamic audit covers the native engine, its C ABI, and the Flutter and JavaScript ownership wrappers. The defects below were reproduced at the audited base revision and fixed in the current working tree. The changes are intentionally uncommitted. This audit is not a claim that the project is free of vulnerabilities.

## Threat model and contract

The engine is an in-process library rather than a network service. A hostile caller must be able to supply transliteration text or FFI arguments. The demonstrated base-revision impacts were invalid reads, use-after-free reads, invalid enum values, or memory growth. No arbitrary write or code execution was demonstrated.

| Boundary | Current behavior |
| --- | --- |
| C ABI `transliterate` | Null pointers and option bits outside the defined `0x7f` mask return null. Other pointers must reference accessible NUL-terminated strings; the first NUL terminates each argument, including text. Text before that NUL is expected to be well-formed UTF-8. Unknown or identical scripts return null. |
| C ABI `isScriptSupported` | Null returns zero. Other pointers must reference accessible NUL-terminated strings; the first NUL terminates the name. Unknown names return zero. Virtual source `indic` is handled by wrappers and is not a concrete dictionary reported as supported. |
| C ABI `releaseBuffer` | Null is accepted. A live result must be released exactly once through this function. |
| Native C++ API | Nonempty views and backing storage must remain valid for the call; views are length-based and can contain embedded NUL bytes. Input is expected to be well-formed UTF-8. C++ option values must combine defined flags. The output-string overload appends and snapshots views that overlap its allocated storage. |
| Size and allocation | There is no explicit input/output length limit. The raw UTF-8 builder checks growth arithmetic and reports `realloc` failure. Allocations by `std::string` and other standard containers inside `noexcept` paths can still terminate on `std::bad_alloc`. |
| Malformed UTF-8 | UTF-8 is not validated. Marker filtering now bounds inferred character lengths and preserves incomplete bytes. Other engine behavior for malformed input is outside the supported contract. |
| Flutter and JavaScript FFI | Argument strings remain alive through the native call. Both wrappers copy native output and call `releaseBuffer` from a `finally` block. |
| Concurrency | Readers, writers, and output buffers are local; generated maps are immutable. Eight threads each completed 100 conversions under sanitizers. `inditransLogger` is not used in the conversion path. |

## Finding summary

| ID | Severity | Reachability and impact | Affected surface | Status |
| --- | --- | --- | --- | --- |
| AUDIT-01 | Low | Requires malformed UTF-8, which is outside the documented input contract but was accepted by raw byte APIs; caused an out-of-bounds read and sanitizer termination. | C++ views, C ABI input, marker filter | Fixed and regression tested |
| AUDIT-02 | Moderate | A valid C++ call could pass an input view backed by its output string; growth caused a use-after-free read and crash. | `std::string&` C++ overload | Fixed and regression tested |
| AUDIT-03 | Moderate | Every successful Flutter conversion leaked its returned native buffer; repeated calls could grow process memory. JS leaked only if copying threw. | Flutter and JavaScript FFI | Fixed with guaranteed release |
| AUDIT-04 | Moderate | Null pointer subtraction occurred on successful builder initialization; injected `realloc` failure led to null pointer arithmetic and a write. | Native output builder and C ABI output | Fixed and failure injected |
| AUDIT-05 | Informational | The short replacement defect is in an internal helper with no production call sites; the reproducer only reaches the helper directly. | `Utf8StringBuilder::replace` | Fixed and regression tested |
| AUDIT-06 | Low | A default empty `string_view` was accepted by the public C++ API and reached null pointer arithmetic; no sanitizer report or crash was observed. | Native C++ API and UTF string helpers | Fixed and regression tested |
| AUDIT-07 | Low | Invalid C ABI calls (null strings or undefined option bits) reached UB; ordinary wrappers do not make these calls. | C ABI boundary | Fixed and regression tested |

Pre-fix sanitizer signatures were captured against the archived `fdaf6b5` source: ASan reported a stack-buffer-overflow (`READ of size 3`) for the one-byte `E2` marker input and a heap-use-after-free (`READ of size 1`) for output aliasing. UBSan reported unsigned-offset pointer overflow for short replacement, a non-zero offset applied to null when injected `realloc` failed, null pointers passed to `std::string_view`, and invalid enum value `4294967295` for `~0ul`. The traces are reproducible with `bash native/security/reproduce_base.sh`; the minimized cases are also kept as passing modes in `native/security/reproduce_findings.cpp`. The empty-view pointer arithmetic was established by source inspection and was not diagnosed by the sanitizer run. LeakSanitizer is unavailable in this container, so the original Flutter leak was established from the wrapper and binding ownership contract rather than a local leak trace.

## Findings fixed

### AUDIT-01 — Truncated UTF-8 marker-filter read

**Severity: Low. Reachability:** malformed UTF-8 through raw byte APIs; malformed input is outside the documented contract. **Impact:** out-of-bounds read and process termination. **Surface:** `SpecialMarkerFilter::stripSpecialMarkers` called from C++ and C APIs.

At the base revision, the filter used an inferred UTF-8 span without considering the remaining view length. A one-byte `0xE2` input reached a marker search as a three-byte view. ASan reported a stack-buffer-overflow (`READ of size 3`); raw marker-filter fuzzing also found a heap-buffer-overflow.

The filter now clamps each inferred span to the available bytes and copies incomplete or invalid sequences through unchanged. `truncated-marker` and the raw-byte fuzzer are retained as passing regressions.

### AUDIT-02 — C++ output alias invalidated input

**Severity: Moderate. Reachability:** a valid native C++ call with input backed by the output string. **Impact:** use-after-free read and process termination. **Surface:** the `std::string&` overload.

At the base revision, the overload could append into the same string that backed its input view. A growth reallocation freed bytes that `InputReader` still needed; ASan reported a heap-use-after-free (`READ of size 1`).

Before reserving or writing, the overload now snapshots any input, script-name, or delimiter view overlapping the output's allocated range. The `output-alias` regression compares the aliased result with a separate conversion.

### AUDIT-03 — FFI result buffers leaked

**Severity: Moderate. Reachability:** every successful non-empty Flutter call; JavaScript release was missed only if copying threw. **Impact:** repeat-call memory growth and possible exhaustion. **Surface:** Flutter and JS/Wasm FFI wrappers.

The Flutter wrapper previously copied the C result and returned without calling `releaseBuffer`, leaking one native allocation for each successful conversion. The Dart wrapper now releases every non-null result in `finally`. The JavaScript wrapper uses the same pattern so a failed string copy also releases the result.

`flutter-release` preserves the copy-and-release ownership sequence. LeakSanitizer could not run in this container because ptrace is restricted; this is recorded as unavailable rather than passed.

### AUDIT-04 — Builder null arithmetic and unchecked `realloc`

**Severity: Moderate. Reachability:** every first builder allocation performed null subtraction; injected or real `realloc` failure led to null pointer arithmetic and a write. **Impact:** undefined behavior in ordinary startup and process crash under memory pressure. **Surface:** raw output builder and C ABI results.

At the base revision, `Utf8StringBuilder::grow` subtracted null pointers during its first allocation and overwrote its only allocation pointer with the unchecked `realloc` result. Allocation failure could lead to invalid pointer arithmetic and a write through null.

The builder now tracks used bytes without null subtraction, checks size arithmetic and geometric growth, preserves the old allocation when `realloc` fails, and stops writes after failure. The `TranslitBuffer` overload returns false and the C ABI returns null if builder allocation fails. Regressions inject failure on the first allocation through both APIs and on a later builder growth.

### AUDIT-05 — Short builder replacement underflow

**Severity: Informational. Reachability:** no production call sites were found; the issue was confirmed by direct use of this internal helper. **Impact:** undefined pointer movement and corruption if reused. **Surface:** `Utf8StringBuilder::replace`.

The old implementation subtracted unsigned lengths when replacing more bytes than the replacement contained. It now clamps the removal range, checks the resulting size, and places the write pointer at the new end. UBSan reported an unsigned-offset pointer overflow for `replace(1, 4, "x")` on `abcdef`; the regression verifies the fixed result is `axf`.

### AUDIT-06 — Default empty view null arithmetic

**Severity: Low. Reachability:** valid public C++ call with a default empty view. **Impact:** undefined pointer arithmetic; no sanitizer report or crash was observed. **Surface:** `InputReader` and UTF string conversion helpers.

A default `std::string_view{}` has a null data pointer. `InputReader` previously formed its end pointer by adding zero to null. The reader and UTF string conversion helpers now use a non-null empty sentinel before pointer arithmetic. The default-empty-view regression passes under ASan/UBSan.

### AUDIT-07 — C ABI null pointers and invalid option values

**Severity: Low. Reachability:** callers passing invalid values directly to the C ABI; standard wrappers pass non-null pointers and defined flags. **Impact:** UB and process termination for invalid calls. **Surface:** C ABI `transliterate` and `isScriptSupported`.

At the base revision, null pointers reached `std::string_view` construction, and arbitrary option values were cast to `TranslitOptions`. UBSan reported null passed to a nonnull `string_view` constructor and an invalid enum load for `~0ul`. The C ABI now checks all five string pointers and rejects bits outside `0x7f` before constructing views or casting. `isScriptSupported(nullptr)` returns zero. All null/invalid-option reproducer modes now pass under ASan/UBSan.

## Invariant review and remaining risks

- Trie prefix and compressed-path reads use bounded `[begin, end)` ranges. Generated tries and script maps are immutable compile-time data.
- XML and protected-delimiter searches are bounded by the input view. An unmatched opening delimiter consumes the remainder of the input.
- Tamil marker matching checks input length before reading marker bytes.
- The token stream holds the maximum 15 encoded tokens plus one lookahead token (16 total), from the 12-bit sequence offset in the 16-bit token representation.
- The 3x output-capacity estimate checks multiplication overflow. `SIZE_MAX + 1` in its fallback is unreachable for a valid accessible string view. The security smoke test checks the arithmetic boundary.
- There is no explicit input/output maximum. Very large inputs can consume proportional memory. Raw builder `realloc` failure is recoverable, while allocation failures in standard containers used by `noexcept` code may terminate the process.
- UTF-8 validity is not checked. The API documents well-formed UTF-8 as the supported input; fuzz targets still exercise arbitrary bytes in bounded code paths.

### Compile-time guarantees and release checks

Generated script and trie tables are `constexpr`/immutable. The 12-bit token sequence offset limits encodings to at most 15 tokens; `InputReader::BufferCapacity` is compile-time set to that maximum plus one lookahead slot. Its `assert(requiredSize <= Lookahead)` is debug-only, but callers request a single lookahead token and the packed encoding cannot yield a longer sequence. Packed prefix-page and compressed-path accesses compare their byte ranges against the input end pointer at runtime, including release builds. C ABI null/option checks, UTF-8 marker bounds, output-capacity multiplication, and builder growth/failure checks also execute in release builds. The code relies on no runtime trie construction or mutable reader/writer cache.

## Static and dynamic checks

Static analysis completed without diagnostics:

```sh
clang++ --analyze -std=c++23 -I native/src \
  -Xanalyzer -analyzer-output=text native/src/inditrans.cpp
clang-tidy -checks='clang-analyzer-*' native/src/inditrans.cpp -- \
  -std=c++23 -I native/src
```

The sanitized regressions run with:

```sh
bash native/security/run_regressions.sh
```

They cover truncated marker bytes, output aliasing, null/invalid C ABI inputs, FFI result ownership, short replacement, first and later `realloc` failure through C++ and C ABI paths, and a default empty view. The broader smoke test covers output growth, XML/protected spans, 64 KiB expansion, capacity arithmetic, buffer reuse, and concurrent calls. The local ASan/UBSan run passed. LeakSanitizer is unavailable due to the container's ptrace restriction.

The regular native suite passed, including the newly added security cases. The final focused security regressions passed under ASan/UBSan. The standalone Wasm build and smoke test passed. The Node.js suite passed all 95 tests. Dart analysis found no issues, and all 95 package Dart tests passed after building the Linux native library. The Flutter launcher itself cannot update engine stamp files in the read-only SDK cache, so the package tests were run directly with the bundled Dart executable.

Six libFuzzer targets and fixture-derived corpora are maintained under `native/security/`: end-to-end transliteration across scripts/options, bounded UTF-8 decoding, raw marker filtering, end-to-end preservation/delimiters, focused Tamil-marker parsing, and C ABI arguments/options. Build them with:

```sh
bash native/security/build_fuzzers.sh
```

After the fixes, bounded 10 second runs with `-max_len=4096`, `-timeout=5`, and `-rss_limit_mb=1024` completed without a crash:

| Target | Executions | Peak RSS |
| --- | ---: | ---: |
| Marker filter | 706,130 | 441 MiB |
| UTF-8 helpers | 475,058 | 353 MiB |
| Preservation | 126,619 | 327 MiB |
| Transliteration | 73,346 | 318 MiB |
| Tamil marker | 1,145,830 | 164 MiB |
| C ABI | 2,592,145 | 460 MiB |

Use `-max_total_time=10` for bounded runs or `-max_total_time=60` for extended runs:

```sh
for target in marker_filter utf8 preservation transliterate tamil_marker c_abi; do
  ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 \
    "out/security/fuzz_${target}" -verbosity=0 -print_final_stats=1 \
    -max_total_time=60 -max_len=4096 -timeout=5 -rss_limit_mb=1024 \
    "out/security/corpus/${target}"
done
```

Each target also completed a 60 second extended run against the fixed source and seeded corpus without a crash:

| Target | Executions | Peak RSS |
| --- | ---: | ---: |
| Marker filter | 5,147,616 | 472 MiB |
| UTF-8 helpers | 3,155,410 | 469 MiB |
| Preservation | 683,727 | 482 MiB |
| Transliteration | 425,432 | 499 MiB |
| Tamil marker | 7,221,798 | 532 MiB |
| C ABI | 12,217,566 | 539 MiB |

Generated corpus mutations and crash files are under ignored `out/security/`. Run `bash native/security/reproduce_base.sh` to archive the audited base, rebuild the minimized vulnerable cases with ASan/UBSan/enum instrumentation, and save their traces under `out/security/baseline-traces/`. The CodeQL workflow runs C/C++ `security-extended` queries on pushes and pull requests to `main`, and weekly. Its hosted result is pending; CodeQL was not available locally.
