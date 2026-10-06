# Idea A: static readers and writers

Idea A uses immutable readers, writer tables, compiled equivalent expansions,
and a Tamil token prefix trie. The current byte matcher uses packed reachable
states and pooled UTF-8 dispatch pages. Its compact representation, Step 9
streaming input, and Step 10 output sinks are measured in the
[compact static experiment](compact-static-experiment.md). The current
[bounded follow-up](compact-static-followup.md) retains direct field access and
only canonicalizes unused empty-range offsets; Wasm is 89.73 KiB with unchanged
code, native table layout, and allocation behavior.

The original design and the 2026-10-03 results below compare against
`dev-pre-compact-metadata` (`4c59278`), with compatibility checked against
`126d17e`. Those measurements describe the earlier flat representation and
remain historical evidence; they are not the current size/performance result.

## Scope and invariants

| Requirement | Implementation | Verification |
|---|---|---|
| Zero reusable reader initialization | `inline constexpr` arrays and descriptors | Startup symbols plus isolated allocation probe |
| Zero reader/writer caches | Static name slots and descriptor arrays | Production call-path and symbol inspection |
| No runtime trie construction | C++23 `consteval` flat-array builder | Compile-time capacity checks and no production mutable trie |
| No runtime equivalent parsing | Generated immutable token spans | Token-level and C ABI differential comparisons |
| No runtime writer binding | Pooled offset/length tables and spans | Metadata probe covers every writer |
| One physical non-Roman dictionary | Shared byte trie, source masks, separate virtual results | Pointer identity and explicit/virtual acceptance tests |
| Better performance than the reference branch | Adaptive dispatch and hoisted source policy | Repeated native and Wasm lookup and whole-call benchmarks |
| Preserve public behavior and C ABI | Existing exports, roles, aliases, token semantics | Shared suites, differential checks, and export verification |

Zero initialization applies to reusable lookup data. Each call still sets its
cursor, grouping, options, and output state and allocates input/output storage.
Wasm instantiation and loader work remain outside this requirement.

## Data generation and construction

[generate_headers.py](../tool/generate_headers.py) reads the existing
[script_data.json](../tool/script_data.json) and
[reader_data.json](../tool/reader_data.json). The latter holds the accent,
exclusive-symbol, and Tamil-prefix constants formerly declared in C++.
[static_scripts.py](../tool/python/static_scripts.py) emits typed entries,
complete token sequences, names, source policies, and writer descriptors into
[script_data.h](../native/src/script_data.h). Generated files must be changed
through this pipeline.

[static_trie.h](../native/src/static_trie.h) builds scratch topology from sorted
unique entries. [packed_trie.h](../native/src/packed_trie.h) retains accelerated
entry points, branching states, and terminal prefixes; nonterminal states
bypassed by compressed paths and terminal leaf records are discarded.
`consteval` packing deduplicates equivalent 64-slot dispatch pages. Source
records, canonical sequence lookup, scratch graphs, and packing intermediates
have no production runtime references and disappear from the release image.

The builders use fixed arrays, loops, and checked integer indices with no
runtime fallback. Capacities choose 16- or 32-bit indices; byte tries reserve
the high bit for a leaf's terminal payload. Nodes with 16-bit indices occupy
eight bytes. Edge ranges, dense-table offsets, page counts, terminal IDs,
source masks, text ranges, and expansion ranges are checked. A future dictionary
that exceeds a capacity must fail generation or compilation.

The implementation uses C++23 with Clang, Emscripten, and MinGW. Testing an
available compiler does not establish that every minimum supported version
implements the constexpr path; record the actual versions with final results.

## Matching layout

All sixteen non-Roman scripts use the same trie and descriptor address.
`ScriptId` selects an explicit-source mask; `ScriptType` controls Indic,
Tamil, or Latin grouping after matching. The virtual `indic` reader uses the
same graph with its own generated result at each terminal. Roman schemes have
separate static dictionaries, sharing the same generic matcher implementation.
Only schemes that need ASCII folding have an additional folded dictionary.

Runtime lookup is bounded by `[begin, end)` and compares unsigned UTF-8 bytes.
It does not construct a UTF-32 string. Dispatch combines:

- A dense 256-slot root and dense tables for sufficiently busy nodes.
- Compact 64-slot tables when all outgoing bytes are UTF-8 continuations.
- One equality for single edges, linear search through at most eight edges,
  and sorted search for larger sparse nodes.
- Ragged two- and three-byte prefix pages that jump to existing trie states.
- Comparisons of two or three unique edges with no intervening terminal.
- A high-bit terminal ID in dispatch and compressed paths; leaves have no node
  or edge records.

The prefix pages store compact states or leaf terminal IDs rather than decoded
Unicode scalars. Dense, prefix, and triple dispatch share the same pooled page
storage. Compressed paths omit nonterminal intermediate states and fallback
edges: a mismatch or truncated path preserves the last accepted prefix.
Ordinary sparse edges remain for branching states. The Tamil token-key matcher
uses the original generic flat trie.

The scanner chooses one of four policies once per call: Roman, folded Roman,
explicit non-Roman, or virtual Indic. This avoids policy dispatch per character
without generating a transliterator for every script pair. A separate raw-text
helper keeps XML/protected-span handling outside the hot loop and evaluates its
XML option there. Source-mask,
primary-sequence, virtual-sequence, and variant-range fields are separate arrays
indexed by terminal for leaves and by compact state for branching nodes.
Parallel branching fields avoid a dependent node-to-terminal load. Path payloads
are also indexed by compact state. The virtual reader loads only its sequence
field. A shared sparse-transition helper prevents inlining four copies of the
same search code into the policy functions.

Explicit readers also use subtree source masks at three-byte prefix pages.
A rejected subtree can stop immediately. When a subtree belongs to one source,
its terminals need no repeated source-mask comparison. These tables derive
from the same terminal variants and do not introduce per-script graphs.

## Longest accepted prefix and source variants

Each non-Roman terminal contains a primary sequence with its accepted-source
mask, an independently selected virtual result, and a range of exceptional
source variants. Masks merge only when the complete sequence agrees, including
script type and every expansion token.

Selection occurs during traversal:

```text
best = no match
walk the shared graph:
    if this terminal is accepted by the source policy:
        best = its sequence and consumed byte length
return best
```

An excluded longer spelling must not hide an accepted shorter prefix. Rejected
terminals do not stop traversal unless the entire subtree is excluded. Compound
spellings, Tamil superscripts, decomposed nukta, joiners, and equivalent strings
continue through terminal boundaries. Bounded tests cover a compound spelling,
its truncated suffix, and a rejected longer terminal.

The historical virtual policy is intentionally precise: start with Devanagari
bases and equivalents, then insert the other non-Roman scripts' **base entries**
in case-insensitive canonical-name order. It has no single-script restriction,
but does not import every script's equivalents. Malayalam chillu `ൻ`, for
example, expands explicitly in Malayalam and is absent from `indic`.
Shared spelling collisions retain the original first insertion.

The generator writes a collision audit to ignored
`out/static-lookup-collisions.json`. It records conflicting payloads and winners
rather than imposing one new meaning across scripts.

## Equivalent expansions

The existing Latin uppercase and decomposed alternatives are preserved.
Generation reproduces base insertion order, sorted equivalent processing,
first-wins collisions, explicit `type:index` references, and textual targets
that can use previously registered equivalents. It also preserves the old
shallow extra-token semantics and handling of partially matched targets.

A match returns a checked 16-bit span descriptor: twelve bits of token-pool
offset and four bits of count. Zero denotes no match. The pool contains complete
immutable `ScriptToken` sequences, with an invalid sentinel at offset zero.
The current inventory fits these limits; generation rejects larger ranges.
The scanner emits the lead token and remaining tokens directly from the span.
There is no runtime target tokenization, equivalent parser, or extra-token
vector construction.

`ScriptToken` remains four bytes wide to preserve word-sized copies and the
existing `TokenUnit` layout. Its reserved byte carries no expansion state.
Expansions are metadata attached to a spelling, rather than an index stored
inside every token.

## Writers, names, and Tamil prefixes

Writers use pooled UTF-8 text and checked compact offset/length entries. Each
target has eight class spans, an empty `Ignore` slot, its script type, and Vedic
flag. `writerChar<writerText>` validates the readable UTF-8 literal against the
pool during compilation; runtime lookup reconstructs a borrowed view without
allocation. Empty and out-of-range entries return an empty view. Accent and
exclusive-symbol choices are resolved during generation.

Script names and aliases resolve through a small constexpr open-addressed
table. Its slot hash uses length and selected ASCII-folded characters; full
name equality resolves collisions. Name resolution and glyph folding remain
separate: `iso` folds ASCII glyphs while `ISO` retains its historical policy.
Unknown-name behavior, exact virtual `indic` handling, native support queries,
and wrapper role restrictions remain compatible. The public C exports and
returned-buffer ownership are unchanged.

The six Tamil prefixes compile into a static trie over `TokenUnit` keys.
Per-call state contains a node ID, optional value, and matched length. Matching
preserves script-type conversion, consonant normalization, token equality,
retained state on a miss, and the old terminal-leaf behavior. Neither reader nor
writer initializes a Tamil reader or constructs prefix nodes.

## Alternatives and boundaries

Idea B, Unicode page dispatch with a suffix trie, was considered but is not
implemented. It would require bounded scalar decoding plus page and suffix
layouts with identical source and terminal rules. The user selected Idea A;
its prefix accelerators retain byte-trie semantics and a single physical graph.

Perfect hashing is unsuitable as the main longest-prefix matcher without an
additional prefix mechanism. Generated branch code per spelling would expand
instruction footprint and undermine shared lookup code. Name hashing is small
and independent of glyph matching.

Input uses Step 9 bounded streaming; output uses Step 10 sinks. Grouping and
public buffer ownership preserve the existing behavior.
This work does not resume the separate
[performance-tuning-plan.md](performance-tuning-plan.md).

## Validation and measurement

Compatibility uses the original `126d17e` engine and the shared JSON suite.
[compare_lookup.py](../tool/compare_lookup.py) compares token sequences and final
C ABI bytes across base/equivalent spellings, aliases and source-name casing,
all targets, option combinations, mixed text, Tamil prefixes, and protected
spans. Token probes help detect differences a writer could conceal. Truncated
input is also checked directly with bounded matcher tests and sanitizers.

Performance uses `dev-pre-compact-metadata` at `4c59278`, with identical compiler
and benchmark flags. Native runs are sequential, pinned to the same CPU, and
alternate old/new order across five repetitions. Compare every workload and
output hash. Cold measurements use 501 fresh processes per path; their timed
region excludes process launch and returned-string destruction. Warm short-call
measurements include destruction. Allocation measurements distinguish first
calls, warmed calls, and isolated reusable metadata access.

The validation covers:

1. Native lookup for every non-Roman source and `indic`, with accepted keys,
   compounds, misses, and source-rejected keys.
2. Native short-call and throughput comparisons against the reference branch,
   preserving output hashes and evaluating the default 5% regression gate.
3. Fresh-process cold latency and cold/warm allocation counts.
4. An isolated metadata/readers/writers/Tamil-prefix probe showing zero allocation.
5. Wasm lookup and whole-call comparisons, distribution smoke tests, and raw size.
6. Native, Dart, Node.js, release, sanitizer, and Windows DLL checks.
7. Generator reproducibility, native section sizes, export/startup inspection,
   and confirmation that compile-time scratch tables are absent at runtime.

Useful commands are `make test`, `make bench-lookup`,
`make bench-lookup-alloc-linux`, `make bench-short-repeat`, `make bench-cold`,
`make bench-compare`, `make wasm`, `make dll`, and `make validate`.
Generated data is refreshed with `python3 tool/generate_headers.py`.
Raw benchmark CSVs, repetitions, and collision reports belong under ignored
`out/`; they are local observations, not release artifacts.

## Results: 2026-10-03

Reference: `dev-pre-compact-metadata`, commit `4c59278`. Host: Linux x86-64
under WSL2, AMD Ryzen 5 4500U. Native release flags are `-std=c++23 -O3
-DNDEBUG`; Wasm uses the repository's `-Oz` release flags. Toolchains tested:
Clang 22.1.8, MinGW GCC 16.2.0, Emscripten 6.0.10, and Node.js 25.8.1.
Minimum compiler versions were not separately installed/tested.

Separate-process end-to-end timings varied substantially and sometimes flagged
inconsistent regressions. The final comparison alternates engines **within each
workload** in one process, pinned to CPU 0. Native engine objects are compiled
with identical flags; defined symbols are renamed before linking both versions
to prevent interposition. The harness calls the unchanged C++ string API and
includes returned-string destruction. Each throughput run uses 31 paired
batches, each short-call run uses 10,000 paired samples, and results below average
five runs. Wasm uses independent module instances, prewarming, paired batches,
and five run medians. All 70 whole-call case/size comparisons pass the 5%
regression and output gate; every paired mean below improves.

Negative percentages mean faster. Large native inputs are approximately 1 MiB;
large Wasm inputs are approximately 64 KiB. These scopes differ intentionally
because the standalone Wasm memory budget limits larger staged inputs.

| Workload | Native short p50, reference → A (µs) | Native 1 MiB change | Wasm 64 KiB change |
|---|---:|---:|---:|
| indic-to-indic | 1.225 → 1.198 | -9.7% | -16.9% |
| tamil-output | 1.569 → 1.493 | -5.6% | -13.8% |
| latin-input | 1.240 → 1.098 | -18.3% | -13.7% |
| virtual-indic-to-latin | 1.208 → 1.138 | -7.1% | -20.7% |
| virtual-indic-to-indic | 1.241 → 1.160 | -5.3% | -19.8% |
| virtual-indic-to-tamil | 1.565 → 1.467 | -3.9% | -17.2% |
| latin-output | 1.213 → 1.192 | -8.3% | -17.3% |
| expansion-heavy | 0.317 → 0.286 | -3.1% | -5.9% |
| protected-spans | 0.489 → 0.395 | -9.6% | -2.4% |
| mixed-protected-spans | 1.030 → 0.930 | -6.6% | -9.2% |

Lookup-only measurements over all 1,574 union spellings include compounds and
source-rejected keys. Five runs give 49–59% faster native lookup for explicit
sources and 30% for `indic`; Wasm gives 55–69% for explicit sources and 46% for
`indic`. This rejected-key-heavy corpus is distinct from the whole-call suite.
Checksums match for all seventeen readers. They share one 2,296-node non-Roman
trie; all current reader indices are sixteen bits.

| First call, 501 fresh processes | Reference p50 (µs) | A p50 (µs) | Change |
|---|---:|---:|---:|
| cold-devanagari-to-telugu | 64.512 | 13.906 | -78.4% |
| cold-iso-to-devanagari | 81.173 | 15.519 | -80.9% |
| cold-devanagari-to-tamil | 65.874 | 15.198 | -76.9% |
| cold-indic-to-iso | 255.651 | 11.401 | -95.5% |

The isolated metadata/readers/writers/Tamil-prefix probe reports zero
`malloc`, `calloc`, `realloc`, `free`, live, peak, or untracked events, with
checksum 1047. Whole-call first allocations fall from 1,091 → 3 for Devanagari,
1,285 → 5 for Latin, and 5,042 → 3 for virtual Indic. All six cold allocation
modes now equal their warmed measurements. Warm allocation event counts,
requested bytes, peak bytes, and retained bytes match the reference exactly;
these calls still allocate input/output storage.

| Release artifact/section | Reference bytes | A bytes |
|---|---:|---:|
| Standalone Wasm | 80,095 | 154,610 |
| Native shared library, unstripped | 256,912 | 351,248 |
| Native shared library, stripped | 240,352 | 342,424 |
| Native `.text` | 41,891 | 37,497 |
| Native `.rodata` | 10,303 | 156,000 |
| Native `.data.rel.ro` | 64,944 | 49,672 |
| Native `.rela.dyn` | 91,056 | 74,856 |
| Native `.bss` | 200 | 16 |

Sequential clean CMake release builds took 3.737 s for the reference and
5.964 s for A; peak child build RSS was 208,692 and 221,908 KiB respectively.
These are single host-specific build observations, not steady-state benchmarks.
Size growth is the accepted cost of static topology, prefix/path acceleration,
and compiled payloads; text and relocation storage both shrink.

Final compatibility verification passed 275,616 token comparisons and 586,422
C API comparisons against `126d17e`, including custom delimiters, unclosed spans,
XML precedence, source restrictions, casing, aliases, and all option bits.
C++ tests, Dart's 47 shared cases, Node.js's 47 shared cases, Dart analysis,
release validation, and Wasm smoke checks pass. Both JS distributions contain
the same rebuilt artifact. The generator reproduces all six checked outputs.

AddressSanitizer and UndefinedBehaviorSanitizer pass. LeakSanitizer is disabled
because process tracing in the sandbox prevents it from operating. MinGW
compilation, C exports, and static runtime dependency checks pass; Wine is
unavailable, so the optional Windows runtime smoke test was not executed.

Release symbol inspection finds no reader caches, runtime script/equivalent
parsers, mutable trie builders, initialization guards, or emitted constexpr
scratch/entry arrays. The sole native `.init_array` entry is `frame_dummy`.
The standalone Wasm imports nothing and retains the required C exports.

Local CSVs and raw repetitions are under `out/static-lookup-*`. Final native
whole-call results use `*-interleaved.csv` and `*-interleaved-short.csv`; final
Wasm whole-call results use `*-wasm-interleaved.csv`. Older separate-process
captures remain labeled separately and are not the final performance gate.
The generated comparison harnesses are saved beside these local measurements.
