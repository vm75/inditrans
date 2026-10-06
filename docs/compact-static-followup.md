# Compact static data: bounded follow-up

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
| Node state | 16 | 508 non-Roman / 110 max Roman | 10 bits | High bit is reserved for leaves; keep native 16 |
| Sparse edge offset | 16 | 312 | 9 bits | A 12-bit field alone would not shrink aligned nodes |
| Terminal ID / Roman sequence | 16 | 4569 | 14 bits | Leaf encoding reserves index high bit |
| Node edge count | 14 + 2 path bits | 69 | 8 bits plus path flag | Changing 8-byte nodes requires hot layout changes |
| Dispatch offset | 15 + continuation flag | 2112 | 12 bits plus flag | Native 16 avoids awkward decoding |
| Alternative begin | 16 | 88 | 7 bits | 12/4 word supports growth and direct decode |
| Alternative count | 16 | 1 | 1 bit | Canonical empty range removes dead offsets |
| Source primary sequence | 16 | 8390 | 14 bits | Top rejection flag needs bounds checks |
| Graph ID | 16 | 10 | 4 bits | Only 26 source records; no multi-KiB gain |
| Writer offset | 16 | 6701 | 13 bits | Use three byte logical offset/length |
| Writer length | 8 | 13 | 4 bits | Three byte entry already removes padding |

These gates bound the campaign: try direct payload encodings and writer padding;
retain branch/path parallelism and readable source declarations. No entropy
compression, runtime metadata construction, decompression, general maps, or
per-script executable specialization is warranted by this inventory.

Analysis artifacts and probe sources are retained under ignored `out/followup/`:
`probe.cpp`, `probe.csv`, `inventory.json`, `reference.map`,
`reference-wasm-inventory.json`, `union.cpp`, `roman_data_cost.py`, and both
data-only Wasm artifacts. They use the existing production builders; they are
not a new runtime benchmark framework.
