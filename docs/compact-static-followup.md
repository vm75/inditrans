# Compact static data: bounded follow-up

## Decision

Recommended experimental checkpoint: `f79e14e` on
`experiment/compact-static-script-data`. Standalone Wasm is **91,883 B /
89.73 KiB**, down **6,516 B / 6.36 KiB / 6.62%** from `c3d5480`.
This is a clear replacement for the previous compact-static checkpoint, a
useful size improvement, and the practical 85–90 KiB floor for the
examined representations. The campaign does **not** establish another 10–25 KiB
of performance-equivalent savings. Smaller 78–85 KiB layouts were built and
measured, then reverted when they failed the runtime priority.

The final layout keeps the original four-byte alternative ranges and canonical
zero offsets for empty ranges. The entire 27,435-byte Wasm code section is
byte-identical to the reference; only serialized data changes. Source masks, explicit/virtual
sequence arrays, branching
payloads, compressed paths, and aligned writer entries retain direct access.
Readable generated declarations and the Step 9/10 engine pipeline are unchanged.
No Step 11, runtime metadata builder, cache, decompressor, constructor, or guard
was added. All prior commits and failed experiments remain in history.

## Clean reference and methodology

Reference `c3d5480` preserves every earlier checkpoint on
`experiment/compact-static-script-data`. Fresh clean snapshot
`out/perf/40-compact-static-reference-*` has five full throughput/short-call runs,
501 fresh-process samples per cold path, and all six cold/warm allocation modes.
Standalone Wasm rebuild and a separate link-map build agree byte-for-byte.

| Reference metric | Value |
|---|---:|
| Standalone Wasm | 98,399 B / 96.09 KiB |
| Code section payload | 27,435 B |
| Data section payload | 70,573 B |
| Initialized segment bytes | 67,143 B |
| Data segment encoding/framing | 3,430 B |
| Primary cold / warm allocations | 7 / 7 |
| Primary cold / warm allocated | 1,050,181 / 1,050,181 B |
| Primary cold / warm requested peak | 1,049,821 / 1,049,821 B |
| Retained live bytes | 0 |

The reusable-metadata probe reports zero malloc/calloc/realloc/free/live/peak/
untracked values and checksum 1047. Latin and expansion peaks remain ~6 MiB and
~3 MiB; ~1 MiB describes the primary workload, not all inputs.

Toolchains: Clang 23.1.1, Emscripten 6.0.10, native `-std=c++23 -O3 -DNDEBUG`,
repository standalone `-Oz` flags; Intel Core Ultra 9 185H, Linux WSL2, CPU 0.
The earlier [experiment report](compact-static-experiment.md) and `out/report.md`
remain historical evidence. The fresh reference is the gate for this campaign.

## Wasm inventory before experiments

A native object size does not predict serialized Wasm savings. The release link
map locates each object; decoding the actual data segments separates allocated
storage, emitted initialized bytes, and nonzero bytes. Addresses were checked
against instantiated Wasm memory. Framing is accounted separately above.

| Object | Storage B | Initialized payload B | Nonzero B |
|---|---:|---:|---:|
| Ten Roman tries | 29100 | 10919 | 6729 |
| Shared non-Roman trie | 15716 | 10704 | 6442 |
| nameSlots | 256 | 94 | 31 |
| emptyTriplePages | 1024 | 0 | 0 |
| sourcePrefixes | 4096 | 121 | 45 |
| sourceMasks | 6300 | 6296 | 1818 |
| primarySequences | 3150 | 3150 | 3147 |
| variantRanges | 6300 | 5894 | 1562 |
| sourceAlternatives | 712 | 710 | 267 |
| branchMasks | 2032 | 1033 | 283 |
| branchSequences | 1016 | 540 | 528 |
| branchVariants | 2032 | 979 | 251 |
| sequenceTokens | 1940 | 1940 | 1164 |
| indicSequences | 3150 | 3128 | 3085 |
| branchIndicSequences | 1016 | 534 | 526 |
| writerChars | 10808 | 10808 | 7967 |
| writerText | 6705 | 6705 | 6704 |
| tamilTrie | 296 | 265 | 132 |
| names | 372 | 372 | 123 |
| readerTries | 352 | 352 | 171 |
| readers | 636 | 546 | 250 |
| writers | 1976 | 1641 | 843 |

Fundamental data includes token meanings, writer UTF-8 strings, aliases, and
source acceptance. Topology and masks repeat some information. Parallel branch
payloads and state-indexed paths are deliberate performance duplication.
Zero-filled maps/path records serialize cheaply. Writer entry padding and
meaningless offsets in empty alternative ranges serialize less cheaply.

## Roman overlap: analysis before implementation

All ten current Roman schemes use the same physical graph for normal and
folded mode (`normalGraph == foldedGraph`). ASCII folding is a matcher policy;
there are no additional folded graph objects to remove.

| Scheme | Spellings | Raw nodes | Packed nodes | Sparse edges | Dispatch slots | 64-slot pages | Paths | Leaves | Storage B | Initialized B |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| hk | 96 | 104 | 31 | 40 | 320 | 5 | 2 | 71 | 1364 | 581 |
| iast | 223 | 295 | 106 | 155 | 832 | 13 | 26 | 163 | 5068 | 1942 |
| ipa | 166 | 287 | 108 | 99 | 1088 | 17 | 57 | 116 | 5388 | 2010 |
| iso | 220 | 294 | 110 | 151 | 960 | 15 | 27 | 154 | 5372 | 1975 |
| itrans | 122 | 146 | 52 | 80 | 320 | 5 | 2 | 92 | 1860 | 915 |
| readableLatin | 84 | 97 | 33 | 42 | 320 | 5 | 3 | 61 | 1404 | 566 |
| slp1 | 95 | 103 | 23 | 31 | 320 | 5 | 2 | 78 | 1200 | 468 |
| titus | 120 | 177 | 65 | 84 | 832 | 13 | 23 | 88 | 4128 | 1220 |
| velthuis | 113 | 126 | 35 | 45 | 576 | 9 | 3 | 88 | 1960 | 681 |
| wx | 96 | 108 | 32 | 34 | 320 | 5 | 4 | 71 | 1356 | 561 |

Raw nodes include the root; packed nodes include the root and reachable prefix/
branch states; leaves count terminal nodes without children; paths count active
compressed paths, not reserved zero path slots. Tables use the current builder.

The ten graphs contain **1,335 spelling occurrences / 517 unique spellings**:
818 duplicate occurrences (61.3%). Separate raw topology has 1,737 nodes;
union raw topology has 722. Separate packed storage totals 29,100 B; the
analysis-only union uses 10,552 B (264 nodes, 478 sparse edges, 1,664 dispatch
slots, 56 active paths). Native topology reduction is 63.7%, before payloads.

| Number of schemes sharing spelling | Unique spellings |
|---|---:|
| 1 | 276 |
| 2 | 97 |
| 3 | 55 |
| 4 | 12 |
| 5 | 5 |
| 6 | 10 |
| 7 | 7 |
| 8 | 10 |
| 9 | 12 |
| 10 | 33 |

187 spellings shared by multiple schemes have one common sequence; 54 shared
spellings have different sequences by scheme; 276 belong to one scheme.
There are 603 distinct (spelling, sequence) groups and 86 exceptional groups
beyond a chosen common sequence. Numerically identical sequence IDs represent
identical complete token sequences in the current canonical pool.

Estimated terminal storage using 16-bit ten-scheme masks/sequence IDs:

| Selection layout | Storage B | Estimated native saving including union |
|---|---:|---:|
| variant_range | 4480 | 14068 |
| common_exception | 3446 | 15102 |
| fixed_two_variants | 4136 | 14412 |
| scheme_matrix | 10340 | 8208 |

The fixed-two-variant estimate is only a size illustration; terminals with more
than two sequences would need overflow storage. The scheme matrix needs
10 × 517 entries, defeating much of the sharing. The common/exception estimate
uses six-byte terminals, four-byte exceptional variants, and checked 12/4 ranges.

Crucially, native saving is not the gate. An **analysis-only data accessor** was
compiled with the normal Emscripten `-Oz`/section-GC pipeline for (a) all separate
packed graphs and (b) union topology plus common/exception payloads. It retains
the raw immutable objects but makes no runtime engine change:

| Analysis artifact | Total B | Code B | Data section B |
|---|---:|---:|---:|
| Separate Roman graphs | 13,045 | 82 | 12,775 |
| Union plus terminal selection data | 9,275 | 75 | 9,013 |
| Saving | **3,770** | 7 | **3,762** |

This estimate includes zero-run serialization and segment framing. Actual engine
integration would also add script-aware terminal selection code/accesses.
**No-go:** the realistic saving is <5 KiB, below the required 10 KiB. The union
is not implemented in the engine. No new Roman selector or per-script code is
introduced. The quantitative analysis is retained even though the idea stops.

## Source payload distributions and gate

| Property | Value |
|---|---:|
| count | 1574 |
| equal_sequences | 1543 |
| primary_zero | 0 |
| indic_zero | 31 |
| both_zero | 0 |
| maximum_alternative_begin | 88 |
| maximum_alternative_count | 1 |
| maximum_sequence | 8390 |
| unique_masks | 19 |

Alternative counts: 1,485 terminals have none, 89 have one, none have multiple.
All 31 unequal virtual sequences are zero; every nonzero virtual sequence equals
the primary sequence. Source-mask frequencies are:

| Mask (hex) | Terminals |
|---|---:|
| 0x3 | 105 |
| 0x4 | 105 |
| 0x8 | 120 |
| 0x10 | 104 |
| 0x20 | 103 |
| 0x800 | 108 |
| 0x1000 | 102 |
| 0x2000 | 26 |
| 0x4000 | 104 |
| 0x10000 | 106 |
| 0x40000 | 109 |
| 0x80000 | 3 |
| 0x82000 | 89 |
| 0x100000 | 106 |
| 0x19683b | 2 |
| 0x200000 | 104 |
| 0x200004 | 1 |
| 0x400000 | 102 |
| 0x79783f | 75 |

Empty alternatives retain an unused cumulative `begin` offset, so range fields
emit 5,894 B for terminals and 979 B for branches despite only 89 alternatives.
A checked 12-bit begin/4-bit count word plus canonical zero for empty ranges is
a realistic >3 KiB opportunity. It preserves parallel state fields and needs
only shifts/masks, with no new dependent lookup. This passes the payload gate.

A separate potential opportunity is equal virtual sequences: the maximum actual
sequence ID is 8,390, leaving its top bit unused. A checked virtual-rejection bit
in primary payloads could remove separate virtual arrays without another load.
It requires measuring explicit/virtual hot paths; no performance equivalence is
assumed from the distribution alone. Pooling 19 repeated masks would add a
common dependent load and is lower priority than these direct encodings.

## Writer, prefixes, dispatch, and width audit

Writer entries: `sizeof(WriterChar) == 4`, 2,702 entries, max offset 6,701,
max length 13. The full 10,808-byte table is initialized in Wasm. One padding
byte per entry accounts for 2,702 B. Three direct byte fields / shifts have a
credible ≥2 KiB saving, without loops or variable-length decoding: gate passes.

The 1,024 source-prefix masks have 33 nonzero entries and 16 unique nonzero
masks. Five unique 64-entry pages would reduce native storage, but only **121 B**
currently serialize. Lookup occurs once per eligible three-byte non-Roman
spelling through `SourceSelector::acceptsPrefix`. A new selector/dependent page
load cannot save the required 1–2 KiB: no-go, retain direct addressing.

Dispatch has 133 pooled pages across graphs, 104 unique by exact content,
29 duplicate pages. Twelve duplicates are all-zero; the 17 nonzero duplicates
contain just 240 duplicate nonzero bytes. Shared roots/page addressing would
also complicate graph offsets. No multi-KiB serialized opportunity is supported:
no cross-graph pooling prototype. Within-graph dense/prefix/triple pages already
share the same pool. Exact 64-slot windows are pooled at compile time.

| Field | Current width | Actual max | Narrowest bits | Decision |
|---|---:|---:|---:|---:|
| Node state | 16 | 507 non-Roman / 109 max Roman | 9 bits | High bit is reserved for leaves; keep native 16 |
| Sparse edge offset | 16 | 312 | 9 bits | A 12-bit field alone would not shrink aligned nodes |
| Terminal ID / Roman sequence | 16 | 12769 | 14 bits | Leaf encoding reserves index high bit |
| Node edge count | 14 + 2 path bits | 69 | 7 bits plus 2 path bits | Changing 8-byte nodes requires hot layout changes |
| Dispatch offset | 15 + continuation flag | 2112 | 12 bits plus flag | Native 16 avoids awkward decoding |
| Alternative begin | 16 | 88 | 7 bits | Zero empty offsets; retain direct full-width field |
| Alternative count | 16 | 1 | 1 bit | Retain direct count after packing slowdown |
| Source primary sequence | 16 | 8390 | 14 bits | Retain separate virtual array after flag experiment |
| Graph ID | 16 | 10 | 4 bits | Compile-time reader input; absent from runtime storage |
| Writer offset | 16 | 6701 | 13 bits | Retain aligned four-byte entry after measurements |
| Writer length | 8 | 13 | 4 bits | Padding retained to protect writer speed |
| Path bytes | 32 | Three bytes / 24 bits | 24 bits | 473 active paths; at most 946 padding B removable |
| Path slot index | Implicit compact state | 507 | 9 bits | Parallel state addressing retained |
| Path/edge child | 16 | 12769 payload plus leaf marker | 16 bits | Shared index carries states and marked terminal leaves |
| Source masks | 32 | 0x79783f | 23 bits | Retain native word after dependent pool regression |

These gates bound the campaign: try direct payload encodings and writer padding;
retain branch/path parallelism and readable source declarations. No entropy
compression, runtime metadata construction, decompression, general maps, or
per-script executable specialization is warranted by this inventory.

Analysis artifacts and probe sources are retained under ignored `out/followup/`:
`probe.cpp`, `probe.csv`, `inventory.json`, `reference.map`,
`reference-wasm-inventory.json`, `union.cpp`, `roman_data_cost.py`, and both
data-only Wasm artifacts. They use the existing production builders; they are
not a new runtime benchmark framework.

## Clean experiment checkpoints and runtime decisions

Times below are percentage changes in per-call time; positive is slower.
Each column is the median across all ten cases at that input scale, not the
headline Indic case. All rows preserve the six-mode cold/warm allocation
results and output hashes. Exploratory rows 41/42 use consecutive snapshots;
rows 50–67 use alternating clean worktrees and five complete untrimmed runs.

| Experiment / checkpoint | Wasm B / KiB | Change vs reference B | Short time | 4 KiB time | 1 MiB time | Memory | Verdict |
|---|---:|---:|---:|---:|---:|---|---|
| Reference `c3d5480` | 98,399 / 96.09 | — | — | — | — | unchanged | reference |
| Alternative ranges `3e70ccf` | 91,708 / 89.56 | -6,691 | +0.38% | +1.74% | +4.15% | unchanged | retain cleanup; reject packing later |
| Shared sequences `db09675` | 87,998 / 85.94 | -10,401 | +2.40% | +6.22% | +5.48% | unchanged | revert after paired/long lookup checks |
| Packed writer `2577983` | 85,299 / 83.30 | -13,100 | +2.49% | +2.17% | +2.59% | unchanged | reject writer stride |
| Pooled leaf masks `2df4224` | 80,773 / 78.88 | -17,626 | +2.84% | +1.20% | +3.55% | unchanged | reject pooled hot lookup |
| Parallel writer `782106a` | 79,955 / 78.08 | -18,444 | +3.91% | +2.61% | +2.78% | unchanged | reject output path |
| Original writer / shared sequences `77ddabd` | 83,475 / 81.52 | -14,924 | +2.73% | +3.02% | +1.12% | unchanged | reject shared sequences and pooled masks |
| Direct sequences / pooled masks `9d86a75` | 87,179 / 85.14 | -11,220 | -0.78% | +2.06% | +0.97% | unchanged | reject pooled masks |
| Packed ranges / direct fields `a3f3725` | 91,708 / 89.56 | -6,691 | +4.80% | +2.64% | +3.17% | unchanged | reject expansion path |
| Canonical empty ranges `f79e14e` | 91,883 / 89.73 | -6,516 | -3.16% | +0.01% | -1.30% | unchanged | recommended |

Roman union remains analysis-only: 3,770 B estimated net saving, below its
10 KiB implementation gate. Prefix pooling, cross-graph dispatch pooling, and
narrower residual fields fail their serialized-size gates. No runtime variants
of those designs were introduced.

The three-byte writer increased 1 MiB Latin-input time by 5.29%, with every
pair slower. Its offset getter already compiles to one unaligned word load;
adding memcpy would not remove the stride/alignment cost. Separate offset and
length arrays with compact class ranges reduced size further, but increased
virtual-to-Tamil 1 MiB time by 8.27%, with all five pairs above 3%. Both layouts
were reverted to the original four-byte entry and precomputed class spans.

Mask pooling introduced a dependent leaf-mask load; compact-state branching
masks stayed direct. The mixed protected case at 4 KiB increased by 6.09% in
the first paired campaign and 6.16% after restoring the original writer and
separate sequence arrays; four of five pairs exceeded 3% in each campaign.
That repeatable whole-call cost fails the performance priority. Direct masks
were restored. The 85.14 KiB candidate also increased 1 MiB Latin-output time
by 3.53% (four pairs above 3%).

Short accepted-key lookup windows initially produced scheduling spikes, such
as 49 ns for a case otherwise near 5–7 ns. All original results are retained.
A supplemental run reuses the existing lookup loop with filtering outside the
timer, 100,000 repetitions instead of 1,000, and five alternating runs. Shared
sequence flags increased virtual Indic lookup from 5.65 to 5.95 ns (+5.3%);
all five pairs were slower. Plain sequence arrays retain predictable access.
The packed-range-only finalist also increased expansion-heavy 1 MiB time by
6.79%, with all five pairs above 3%; Latin input increased by 4.64% (four pairs
above 3%). Keeping the original begin/count fields while only zeroing unused
empty offsets costs just 175 B more Wasm and restores byte-identical runtime
code. Packing was therefore reverted. The inventory isolates the savings:
canonical zero offsets save **6,516 B**, while narrowing the fields saves only
**175 B more**. Narrowing alone fails the multi-KiB gate. No runtime design was
changed solely because of one submicrosecond p50/p95.

## Final whole-call matrix

Untrimmed arithmetic means of five run medians, CPU 0. The table includes all
30 throughput cases; the displayed deltas compare final versus `c3d5480` and
fresh Step 10 `556cb5f`. Hashes and returned-size sinks agree in every cell. Final medians across cases
are -3.16% short, +0.01% at 4 KiB, and -1.30% at 1 MiB versus the compact
reference. The Tamil 4 KiB case is +10.08% (four pairs above 3%); retain that
result explicitly rather than claiming zero regressions. The native benchmark
text **and every runtime metadata symbol address/size are byte-identical** to
reference, as is the release library's 45,479-byte text. Only unused empty-range
offsets change. There is no new instruction, dependent lookup, or changed native
table layout to explain an architectural slowdown; the varying timings limit
performance claims. The recommendation rests on code/layout equivalence and
exact memory/correctness gates, not on selecting favorable timing cells.

| Case | Short vs compact | 4 KiB vs compact | 1 MiB vs compact | 1 MiB vs Step 10 |
|---|---:|---:|---:|---:|
| indic-to-indic | +0.11% | +0.55% | +3.20% | +3.61% |
| tamil-output | +0.25% | +10.08% | +3.73% | +4.98% |
| latin-input | -10.08% | -3.30% | +1.21% | -1.96% |
| virtual-indic-to-latin | +1.85% | +5.36% | -2.21% | -8.71% |
| virtual-indic-to-indic | -3.11% | -7.87% | -1.16% | -7.57% |
| virtual-indic-to-tamil | -1.46% | -0.27% | -2.89% | -10.64% |
| latin-output | -4.23% | +0.28% | -2.52% | -9.34% |
| expansion-heavy | -3.21% | -2.40% | -1.45% | -9.58% |
| protected-spans | -5.00% | -3.07% | -0.74% | -8.82% |
| mixed-protected-spans | -4.28% | +0.29% | -2.52% | -10.09% |

## Short-call latency and isolated lookup

Short-call timings include returned-string destruction. These small p50/p95
values remain sensitive to WSL scheduling; retain all five raw runs and use
whole-call/longer lookup repetitions to decide architecture.

| Case | Reference mean p50 / p95 ns | Final mean p50 / p95 ns |
|---|---:|---:|
| indic-to-indic | 962 / 1589 | 916 / 1330 |
| tamil-output | 1206 / 1495 | 1095 / 1195 |
| latin-input | 918 / 1065 | 913 / 1144 |
| virtual-indic-to-latin | 897 / 1238 | 864 / 932 |
| virtual-indic-to-indic | 885 / 1024 | 881 / 950 |
| virtual-indic-to-tamil | 1098 / 1162 | 1113 / 1384 |
| latin-output | 984 / 1325 | 883 / 962 |
| expansion-heavy | 223 / 263 | 183 / 280 |
| protected-spans | 361 / 415 | 323 / 363 |
| mixed-protected-spans | 780 / 926 | 775 / 840 |

The normal lookup corpus includes misses. The accepted corpus includes only
keys with nonzero sequences for that reader; setup/filtering is outside the
timer. Counts, consumed-length checksums, and token results match. Final accepted
measurements use 100,000 repetitions, as do the longer prototype checks.

| Source | Normal corpus reference / final ns | Accepted corpus reference / final ns |
|---|---:|---:|
| assamese | 3.467 / 3.310 | 7.254 / 7.253 |
| bangla | 5.070 / 4.311 | 7.289 / 7.285 |
| burmese | 3.345 / 3.410 | 7.222 / 7.347 |
| devanagari | 3.061 / 3.098 | 7.109 / 6.896 |
| gujarati | 3.329 / 3.213 | 7.042 / 6.775 |
| gurmukhi | 3.291 / 3.343 | 7.733 / 7.407 |
| kannada | 3.649 / 4.559 | 7.042 / 6.980 |
| khmer | 3.170 / 3.492 | 7.566 / 7.074 |
| malayalam | 3.154 / 3.183 | 7.627 / 7.265 |
| odia | 3.230 / 2.989 | 7.455 / 7.222 |
| sinhala | 3.266 / 3.943 | 6.684 / 6.722 |
| tamil | 3.679 / 3.503 | 9.725 / 9.745 |
| tamilextended | 3.811 / 3.486 | 7.544 / 7.823 |
| telugu | 3.027 / 3.026 | 7.022 / 7.062 |
| thai | 3.264 / 3.110 | 8.485 / 8.020 |
| tibetan | 3.435 / 4.083 | 8.325 / 7.958 |
| indic | 7.679 / 6.827 | 6.949 / 6.855 |

## Memory and fresh-process calls

Cold and warm results agree exactly with the reference for all six modes.
Peak means allocator-probe requested live bytes, not RSS or Wasm linear-memory
capacity. Reusable metadata retains zero allocated/live bytes; checksum 1047.

| Allocation mode | Cold / warm events | Requested B | Peak requested live B | Retained B |
|---|---:|---:|---:|---:|
| devanagari | 7 / 7 | 1,050,181 | 1,049,821 | 0 |
| latin | 9 / 9 | 7,341,708 | 6,292,364 | 0 |
| virtual-indic | 7 / 7 | 1,050,181 | 1,049,821 | 0 |
| expansion | 8 / 8 | 3,147,269 | 3,146,525 | 0 |
| protected | 7 / 7 | 1,050,113 | 1,049,753 | 0 |
| mixed-protected | 7 / 7 | 1,050,180 | 1,049,820 | 0 |

No initialization architecture changed. The final check nevertheless samples
501 fresh native processes per path, measuring the first call rather than OS
process creation. Instantiation/loader costs are outside this measurement.

| Cold path | Reference p50 / p95 ns | Final p50 / p95 ns |
|---|---:|---:|
| cold-devanagari-to-telugu | 9401 / 12926 | 9411 / 13695 |
| cold-iso-to-devanagari | 8891 / 11706 | 9298 / 12480 |
| cold-devanagari-to-tamil | 11029 / 15569 | 10813 / 12991 |
| cold-indic-to-iso | 9411 / 14110 | 9199 / 11969 |

## Final serialized size anatomy

Release rebuild: 91,883 B total; code section 27,435 B;
data section 64,057 B. There are 486 segments containing
60,627 initialized bytes; the remaining data-section
bytes encode segment framing. Storage and initialized payload differ substantially.

| Object | Storage B | Initialized B | Nonzero B |
|---|---:|---:|---:|
| nameSlots | 256 | 94 | 31 |
| emptyTriplePages | 1024 | 0 | 0 |
| packedReaderTrie0 | 15716 | 10704 | 6442 |
| packedReaderTrie1 | 1364 | 581 | 389 |
| packedReaderTrie2 | 5068 | 1942 | 1212 |
| packedReaderTrie3 | 5388 | 2010 | 1055 |
| packedReaderTrie4 | 5372 | 1975 | 1226 |
| packedReaderTrie5 | 1860 | 915 | 586 |
| packedReaderTrie6 | 1404 | 566 | 378 |
| packedReaderTrie7 | 1200 | 468 | 338 |
| packedReaderTrie8 | 4128 | 1220 | 710 |
| packedReaderTrie9 | 1960 | 681 | 446 |
| packedReaderTrie10 | 1356 | 561 | 389 |
| sourcePrefixes | 4096 | 121 | 45 |
| sourceMasks | 6300 | 6296 | 1818 |
| primarySequences | 3150 | 3150 | 3147 |
| variantRanges | 6300 | 351 | 177 |
| sourceAlternatives | 712 | 710 | 267 |
| branchMasks | 2032 | 1033 | 283 |
| branchSequences | 1016 | 540 | 528 |
| branchVariants | 2032 | 6 | 4 |
| sequenceTokens | 1940 | 1940 | 1164 |
| indicSequences | 3150 | 3128 | 3085 |
| branchIndicSequences | 1016 | 534 | 526 |
| writerChars | 10808 | 10808 | 7967 |
| writerText | 6705 | 6705 | 6704 |
| tamilTrie | 296 | 265 | 132 |
| names | 372 | 372 | 123 |
| readerTries | 352 | 352 | 171 |
| readers | 636 | 546 | 250 |
| writers | 1976 | 1641 | 843 |

Roman and non-Roman topology, token sequences, text bytes, and source semantics
are fundamental information. Parallel branching/path fields, separate virtual
results, raw masks, and writer padding spend bytes for direct hot-path access;
this campaign measured the cost of removing several of them. Zero-filled
prefix/page storage already serializes cheaply and offers little Wasm saving.
Readable spelling/semantic source records and scratch builders add no second
copy to the release image; only runtime fields and pooled writer text remain.
Readable `script_data.h` is byte-identical to the reference source.

The remaining 89 alternative records and canonical empty-range tables are
small in serialized form. Three-byte paths could remove at most about 946
padding bytes from 473 active paths; sparse-edge padding totals 1,074 B.
An analysis-only containing-string pool retains 1,406 of 1,851 nonempty unique
writer strings and reduces raw text from 6,704 to 5,344 B: only 1,360 B before
framing/offset effects. None is a demonstrated multi-KiB low-cost opportunity.

## Recommendation, targets, and stopping point

Retain the final direct-field checkpoint as the bounded campaign result.
It is **useful incremental success** (≤90 KiB), with a practical static-data
floor near 89.73 KiB for the examined layouts. It misses the strong ≤85 KiB and
primary ≤80 KiB goals. The smaller prototypes are useful evidence of the size
versus runtime trade-off, and are preserved as independent commits.

- **80 KiB:** another 9.73 KiB is required. The tested route combines mask,
  sequence, and writer compression; it has measured hot-path costs and is not
  justified under the current priority order.
- **75 KiB:** another 14.73 KiB is required. Roman sharing adds only ~3.68 KiB
  before selector costs, and the smaller payload/writer designs already failed
  runtime gates. No supported performance-equivalent route was identified.
- **70 KiB:** another 19.73 KiB is required; no zero-cost structural opportunity
  of that size was found. Do not proceed through complicated hot decoding.
- **60–65 KiB:** another 24.73–29.73 KiB is required. The ~59 KiB typed result
  used runtime metadata reconstruction (~1,091 cold allocations / ~25 MiB peak),
  which does not meet this task's architecture. Its size is not proof that this
  immutable representation can achieve the same target without that cost.

Stop conditions B/D apply: the multi-KiB tested routes introduced repeatable
runtime slowdowns, while the direct-access result lies in the 85–90 KiB range.
This is a practical floor for the inspected candidates, not a mathematical
lower bound. Future work needs a new measured structural opportunity; do not
restart the rejected designs merely to chase a headline size.

## Validation, history, and retained evidence

Final checks: C++ suites (405,251 packed-data assertions), Dart analysis,
Flutter tests, Node.js/Jest tests, release validation, standalone Wasm smoke,
564,634 C ABI differential comparisons, and deterministic regeneration all pass.
Production native symbols contain no metadata builder/cache/guard/global
constructor; Wasm imports nothing and its constructor function is a nop.
`inditrans.cpp`, `inditrans.h`, and `exports.h` remain unchanged from Step 10.
No ABI/binding, version, canonical JSON, or unrelated dependency change occurred.
The original `out/report.md` SHA-256 remains unchanged.

Checkpoints are retained in order: `c3d5480`, `7d73820`, `3e70ccf`, `db09675`,
`2577983`, `2df4224`, `782106a`, `77ddabd`, `9d86a75`, `a3f3725`, and the final checkpoint
above. No branch history was squashed or rewritten.

Raw five-run snapshots live under ignored `out/perf/40–67` prefixes. Original
component runs are archived under `out/followup/archive/`; the accepted-key
sources, controllers, analysis artifacts, link maps, section inventories,
validation logs, and original exploratory results remain under
`out/followup/`. The Step 10 lookup benchmark at its historical commit does not
compile with `Utf8Key::data`; its unchanged whole-call/short/allocation matrices
were measured, and its unsupported lookup target was excluded before any
recorded timing run. No timing run was discarded or trimmed.

These are native runtime measurements. Wasm size, static initialization,
exports, smoke behavior, and Node.js functionality were checked; native timings
do not establish browser/Wasm performance or portable nanosecond guarantees.
