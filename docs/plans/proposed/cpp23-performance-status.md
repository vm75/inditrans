# C++23 performance implementation status

Last updated: 2026-09-29

## Objective and constraints

Continue the user-requested `cpp23-performance` implementation. Do not commit or push
changes. Keep the pronunciation exploration separate. Do not mark the goal complete
until the open validation and acceptance gates are resolved.

## Current state

P1–P4 and P6–P7 have substantial implementation and historical validation recorded
in `cpp23-performance.md`. P5 currently uses immutable generated tables: root edges
are grouped into high-byte pages with deduplicated low-byte bitmaps, rank prefixes,
and compact child ranges; non-root edges remain sorted. `ReaderNode` is 4 bytes,
with a six-bit edge count and ten-bit token ID packed into `edgeMeta`; root-page
count and extras count are stored as bytes in the six-byte `ReaderData`. Compile-time
assertions and generator bounds checks enforce these limits. Root lookup checks the
first and last pages before the range guard, checks the second page directly, and
uses cached lookahead plus `string_view::find` for input scanning. The latest
candidate meets the short-call gate against its immediate predecessor, but a fresh
three-pair comparison against rebuilt `d8b7140` shows a +4.6% median regression on
1 MiB virtual Indic input. The other six 1 MiB medians remain within 3%; 4 KiB
measurements contain large outliers. Compressed Wasm size remains above the +5%
target, so P5 is not merge-ready.
The generator documents and enforces its BMP-only input bound.

No commit or push was made. The current source includes compact ranked root bitmaps,
the hash cleanup, and full-output benchmark checksums. Native tests pass after
regeneration. Generator output was regenerated twice with identical tokenizer-header
hashes.

### Comparison against the performance baseline

The plan's performance baseline is `d8b7140`, the pre-C++23 snapshot used to
measure the performance work. It is not the current merge base: `dev` and `main`
currently meet at `3044baa`. The baseline was extracted to
`/tmp/inditrans-d8-baseline/` because `.git` is read-only and cannot create linked
worktrees. Both builds used Clang 22.1.8, `-std=c++23 -O3 -DNDEBUG -march=native`,
and CPU 2 pinning. Three process pairs ran in alternating order. The table reports
medians across the three run medians. Full-output hashes and sample checksums match.
An additional generated-spelling differential compared 1,048,971 cases across all
64 option combinations and three output modes, plus 19,200 uppercase ISO/IAST
folding cases; the byte streams matched. Main corpus hash:
`61618f0be0df3df9ba194f3df71d9ccd9d912349d7e85e544f5f5ab1ee6cbf4c`;
uppercase corpus hash:
`2812e9517d183cc09a12e482dfe19781e0581e981590d3a985144ec2bf44f17c`.

| Workload | 1 MiB median paired change vs `d8b7140` |
|---|---:|
| Devanagari → Telugu | -2.55% |
| Devanagari → Tamil | -7.33% |
| ISO → Devanagari | -1.36% |
| Virtual Indic → ISO | -9.78% |
| Expansion-heavy | +0.48% |
| Protected spans | -4.57% |
| Mixed protected spans | -6.83% |

These are three alternating, CPU-pinned pairs; output hashes matched. The 4 KiB
median paired changes are -0.79%, -6.51%, -4.12%, +1.86%, -4.54%, -6.17%, and
-5.74%. Ten alternating short-call pairs show p95 changes of -12.59%, -14.80%,
-9.29%, -9.98%, -52.00%, -33.87%, and -12.99%. No measured median regression
exceeds 3%; the long-input and short-call results meet their timing gates.

### Lookup cutoff screen

A focused single-process screen compiled the same source with linear-scan cutoffs
of 0, 8, 32, and 65,535 and compared full 1 MiB output hashes with `d8b7140`.
All hashes matched. Cutoff 8 was the fastest of the current variants in most
cases; forcing binary search (0), extending linear scans to degree 32, or using
linear scans for every node did not remove the regression. The all-linear case
was substantially slower. This screen is diagnostic, not a paired acceptance
measurement, so cutoff 8 remains unchanged pending a better lookup design.

For the requested `dev` versus `main` comparison, the earlier sorted-root build
(before hash-path cleanup) also matched checksums in three pinned runs. It was
45.3–115.6% slower on 1 MiB workloads than `main` (`3044baa`). This comparison
includes all changes on `dev` since the merge base; use `d8b7140` to assess the
performance work itself. The current ranked-root implementation has not been
rebenchmarked directly against `3044baa`; the earlier comparison remains only a
branch-level warning, not current tokenizer acceptance evidence.

### Lookup alternatives measured this turn

- Per-reader root minimum/maximum scalar bounds slightly improved the sorted-root
  build in an unpaired screening run, but three paired runs against `d8b7140` still
  regressed all seven long workloads by 10.0–80.5%; short-call latency also missed
  the gate. The extra bounds were removed.
- A bucket directory for sorted roots was 13–80% slower than `main` on the long
  cases in one screening run. It was removed.
- Eytzinger ordering for root edges passed native fixtures and full short-output
  hashes, but a pinned long run remained 7.7–102.5% slower than `d8b7140`. Short
  p95 regressed 8.2–154.4% in five workloads, so it was removed.
- The high-byte page plus low-byte bitmap/rank path improved 1 MiB throughput by
  roughly 25–40% over the earlier sorted-root candidate in the paired screening,
  while keeping all output hashes equal. Deduplicating bitmap signatures reduced
  bitmap-plus-rank tables from 6,160 B to 2,360 B; paired timing changes were mostly
  within 0–4% of the non-deduplicated version. A packed 10-bit child-table trial
  reduced table bytes but slowed multiple workloads 5–20%, so it was discarded.
- A 256-bit high-byte page directory removed the short page scan but slowed most
  long workloads 4–10%; it was discarded. Binary search only for the 14-page
  virtual root improved that workload's 1 MiB time by about 21%, but regressed its
  short-call p95 by 15% in ten alternating pairs. Passing an input-size guard into
  every lookup avoided that p95 result but materially slowed the long workloads, so
  both variants were discarded.
- A 16-way root-page bucket index added 493 raw bytes and bounded page scans, but
  three alternating pinned pairs against the compact bitmap build regressed six
  1 MiB cases by 1.9–13.2% and six 4 KiB cases by 1.2–18.4%; output hashes matched.
  A one-time input-size dispatch to binary search (inputs at least 8 KiB) improved
  no aggregate gate: across three pairs it regressed the 1 MiB virtual and Tamil
  cases by 23.2% and 10.1%, respectively, with other cases from 1.2% faster to
  2.9% slower. Raising the root cutoff to 13 pages still regressed those same
  workloads by 23.2% and 10.1%. Both trials were removed. The short-call p95 run
  had large outliers even though the binary path was not active for short inputs,
  so it is not treated as a valid short-call comparison.
- `sizeof(StringRef)` is 4 on the native and Wasm builds. A structure-of-arrays
  layout saved 2,632 bytes from the Wasm data section (standalone raw 59,008 B;
  JS/Wasm raw 93,711 B), but regressed Tamil 1 MiB throughput 6.3%. A portable
  three-byte record (explicit low/high offset bytes plus length) kept the same
  data-section reduction (standalone raw 58,824 B; JS/Wasm raw 93,459 B), but
  still regressed Tamil 1 MiB throughput 4.4% in three pinned pairs. Both layouts
  were reverted because of the repeatable Tamil regression. The current compact
  bitmap Wasm artifacts were rebuilt with `gzip -n -9` sizes of 28,933 B standalone
  and 43,435 B for JS/Wasm. The current branch-order build is 61,617 B / 28,941 B
  standalone and 97,187 B / 43,437 B JavaScript/Wasm (raw / gzip); its data section
  remains 41,502 B.
- Reordering root page checks to test the first/last descriptor before the range
  guard improved the previous compact-bitmap build in three alternating pinned
  pairs: 1 MiB changes were -8.4%, 0.0%, -3.8%, -5.5%, -6.0%, -2.7%, and -2.0%
  (workload order from the main table); 4 KiB changes were -4.4%, -6.9%, -0.1%,
  -0.3%, -5.3%, -3.2%, and +0.1%. Ten short-call pairs put p95 changes between
  -19.4% and +2.3% versus the previous bitmap build. Ten direct short-call pairs
  versus `d8b7140` put p95 changes between -45.7% and +3.8%. All full-output hashes
  matched. This branch ordering is retained.
- A virtual-reader page-priority trial moved the common Devanagari and ASCII pages
  to the first two slots and used a compile-time `InputReader` specialization so
  other readers paid no per-token branch. It improved the 1 MiB virtual case by
  4.3%, but regressed Devanagari→Tamil and protected spans by 5.8%, and
  expansion-heavy by 3.8%. A runtime-flag version regressed several nonvirtual
  cases by 8–17%. Both the reordered generated table and lookup paths were removed.
- A direct three-byte UTF-8 root decoder avoided scalar reconstruction while
  retaining bounds and continuation checks. It passed native fixtures and all
  benchmark hashes, but three paired runs regressed 1 MiB Devanagari→Telugu by
  10.2% and Tamil output by 16.2%; virtual Indic improved 8.4%. It also regressed
  4 KiB Tamil by 16.6%. The decoder was removed.
- Widening each root bitmap from eight 32-bit words to four 64-bit words cut the
  bitmap table by 944 raw bytes. Native tests and all benchmark output hashes
  matched. Three alternating pinned pairs against the retained 32-bit layout
  regressed 1 MiB Tamil by 7.6%, virtual Indic by 4.9%, and expansion-heavy input
  by 5.8%; other workloads changed from 2.7% faster to 1.2% slower. Five short-call
  pairs kept p95 changes between -4.0% and +1.9%. The long-input regressions fail
  the gate, so the 64-bit layout and generator changes were reverted.
- Path-compressing unary nonterminal runs removed 99 nodes, but three paired runs
  regressed 1 MiB Tamil by 11.9% and short-call Devanagari p95 by 6.1%; hashes
  matched. Requiring at least two removable nodes reduced the change to 18 fewer
  nodes and 14 paths, saving about 60 raw table bytes before code overhead. Five
  short-call pairs stayed within the p95 gate, but long-run results were noisy and
  the rebuilt Wasm grew by 220 B raw / 93 B gzip standalone and 288 B raw / 63 B
  gzip for JS/Wasm. Both path encodings were removed.
- Packing root-child references into 10 bits reduced that raw table by 3,490 B
  and matched all output hashes, but three alternating pinned pairs regressed 1 MiB
  Tamil by 10.3%; Devanagari, virtual Indic, and mixed protected input also exceeded
  the 3% gate. Short-call p95 stayed within +4.3%. Gzip Wasm grew by 1,060 B
  standalone and 133 B for JS/Wasm, so the packed representation was reverted.
- A byte encoding stored 3,508 of 3,679 terminal references directly and used
  an escape bitmap/rank table for the rest, reducing raw child-reference storage
  by 1,485 B. It matched hashes but regressed all seven 1 MiB workloads by
  2.8–16.4%, with short Latin p95 up 21.4%; gzip Wasm also grew. The encoding was
  removed.
- Reordering token IDs by reference frequency kept the lookup operations and
  array sizes unchanged and saved only 51 B standalone / 21 B JS/Wasm gzip. Three
  pinned long pairs regressed Tamil by 8.7%; ten short-call pairs regressed p95 by
  5.8–8.9% in Latin, expansion-heavy, protected, and mixed-protected cases. The
  generator and token array were restored to traversal order.
- Stopping the bounded root-page scan at the first page greater than the target
  shortened the miss loop in theory, but three pinned pairs regressed 1 MiB Tamil
  by 12.0% and five short-call pairs regressed its p95 by 7.7%; hashes matched.
  This search was reverted in favor of the existing first/last checks and equality
  scan.
- Moving root lookup outside the repeated trie loop improved six 1 MiB workloads
  by 0.1–6.7%, but virtual Indic short-call p95 regressed 9.9%. An inline fallback
  for the 14-page root brought short-call p95 back within +2%, but the paired long
  run regressed protected spans by 3.6% and gzip Wasm grew by 309 B standalone /
  396 B for JS/Wasm. Both loop layouts were reverted.
- A later direct `d8b7140` rerun used matching `-march=native` flags, but the same
  archived long benchmark varied by more than 2× across sessions and paired ratios
  swung sharply with run order. It is not used to replace the earlier stable
  baseline table; repeat the comparison on a quieter machine before treating it as
  current acceptance evidence.
- Interning identical root-page child sequences would remove 1,686 of 4,655 child
  references (3,372 raw table bytes). Three repeated pairs against `d8b7140` matched
  hashes but regressed 1 MiB time by 7.9–19.2% in six workloads; the generator
  change and generated table were reverted to preserve lookup locality.
- Selective interning kept Devanagari, ISO/IAST, and virtual Indic root pages private
  and shared identical sequences only among other readers. This removed 1,193 of
  4,655 child references (2,386 raw bytes); gzip Wasm fell by 267 B standalone and
  206 B in JavaScript/Wasm. Three CPU-pinned candidate/baseline runs matched every
  output hash, but median-of-run medians regressed 1 MiB latency by 8.1% overall
  (up to 24.8%) and 4 KiB latency by 9.8% overall (up to 14.8%). The selective
  sharing change and generated table were reverted because the lookup regression
  outweighed the compressed-size savings.
- A thresholded binary search for readers with more than eight root pages was
  compared with the retained page scan. Results changed sharply between repeated
  runs, including across baseline reruns, so this probe is not acceptance evidence;
  the lookup change was reverted pending a quieter benchmark environment.
- Packing each node's edge count and token ID into four bytes reduced the Wasm data
  section by 1,096 B (40,406 B current versus 41,502 B in the prior six-byte-node
  candidate), with no repeatable throughput regression over 3% versus that layout.
  It saves 1,095 B raw / 204 B gzip standalone Wasm and 1,460 B raw / 43 B gzip in
  JS/Wasm. `ReaderData.rootPageCount` keeps root lookup independent of the packed
  edge count. A trial separating edge scalars and children into two arrays passed
  correctness checks but produced highly unstable paired timings with no reliable
  throughput benefit; it was reverted.
- A compact token-ID lookup result and root-loop peeling failed paired checks and
  remain discarded. The retained `InputReader` pending-match cache avoids repeating
  lookups while extending unmatched runs and after protected spans. Three pairs
  against the packed-node predecessor improved 1 MiB medians by 2.5–13.6% across
  the seven workloads; hashes matched. `string_view::find` for nonempty protected
  span end markers then improved Tamil by 2.0%, protected spans by 12.3%, and mixed
  protected spans by 3.5%, with other cases within 2.5%. Checking the second root
  page directly improved all seven workloads by 1.5–5.2% in another three pairs.
  These changes compose to the current baseline results in the table above.
- Portable three-byte `StringRef` and SoA string-offset layouts were retested against
  the current root-page build. Both passed output checks but regressed several long
  workloads (up to 3.5% and 8.6%, respectively), so the original four-byte
  `StringRef` remains. The three-byte trial also increased standalone Wasm gzip by
  968 B despite reducing raw size.
- Sorting root-child blocks by their child-reference sequence left lookup code and
  table bytes unchanged, but reduced standalone gzip by only 69 B and increased
  JavaScript/Wasm gzip by 111 B. It was reverted.
- Sorting non-root edge blocks by scalar/child sequence saved 103 B standalone
  gzip and 6 B for JavaScript/Wasm. Three alternating pinned pairs matched output
  hashes, but median 1 MiB timings regressed Devanagari→Telugu by 3.45% and Tamil
  output by 3.85%; the ordering was reverted.
- Exact root-map interning shares roots only when terminal and transition signatures
  match. It removed 14 root pages (154→140) and 392 references (4,655→4,263),
  reducing Wasm gzip by 73 B standalone and 244 B for JavaScript/Wasm. Three
  alternating pinned pairs versus the preceding candidate changed 1 MiB medians by
  -1.29% to +0.67%; ten short-call pairs versus `d8b7140` matched hashes and met the
  p95 gate. This generator-only change is retained.
- Exact child-sequence interning after root-map interning removes 1,294 of 4,263
  root-child references (2,588 raw table bytes). Relative to the exact-root-only
  candidate, gzip falls from 28,908 B to 28,617 B standalone and from 43,476 B to
  42,848 B for JavaScript/Wasm. Three paired runs versus that predecessor changed
  1 MiB workload medians by -13.28% to +1.95%; ten short-call pairs changed p95 by
  -15.90% to +1.57%; hashes matched. It is retained provisionally for its size
  reduction. A fresh direct comparison against a newly compiled `d8b7140` baseline
  used three alternating pairs with identical Clang 22.1.8 `-O3 -DNDEBUG
  -march=native` flags and the same benchmark source. All hashes matched. The 1 MiB
  median changes were -1.4% Devanagari→Telugu, -9.3% Tamil output, +1.9% Latin
  input, +4.6% virtual Indic input, +2.0% expansion-heavy, -2.3% protected spans,
  and +1.2% mixed protected spans. The virtual Indic throughput gate currently
  fails; 4 KiB measurements were noisy, including >100% outliers in two paired
  protected-span samples.
- A 12-bit encoding for root-child references reduced their generated table from
  5,938 B to 4,454 B after child-sequence interning. Three alternating pairs
  matched all output hashes but regressed every 1 MiB workload by 5.4–9.1% at the
  median. The packed encoding was removed; 16-bit references remain.
- Sharing main terminal-token entries across Indic, Tamil, and Latin reduced the
  generated token table from 471 to 195 entries, nodes from 548 to 536, and edges
  from 803 to 791. Gzip fell by 967 B standalone and 1,116 B for JavaScript/Wasm,
  but three alternating pinned pairs regressed all seven 1 MiB workloads by
  5.5–13.8%; hashes matched. Restricting suffix sharing to the same script type
  restored node/edge counts but still showed 3.3–10.1% regressions in a single
  screen. The optimization was removed because it violates the throughput gate.
- `ScriptReaderMap::lookupToken` accounted for about 52% of sampled runtime in an
  earlier gprof run. The retained input-scan and page lookup changes now meet the
  measured throughput gates; the remaining acceptance blocker is Wasm size.

### Artifact state

Checked-in artifacts were rebuilt from the compact ranked-root, packed-node candidate. The
baseline was freshly rebuilt from `d8b7140` under the same Emscripten 4.0.3
release flags (`-Oz -flto`, C++23). Gzip used `gzip -n -9`.

| Artifact | `d8b7140` | Current candidate | Change |
|---|---:|---:|---:|
| Standalone Wasm raw | 54,353 B | 57,512 B | +5.8% |
| Standalone Wasm gzip | 24,312 B | 28,617 B | +17.7% |
| JS/Wasm raw | 86,855 B | 91,715 B | +5.6% |
| JS/Wasm gzip | 37,609 B | 42,848 B | +13.9% |

The data section is 36,965 B in the current standalone Wasm and 17,490 B in the
same-flags baseline. Both compressed outputs exceed the +5% target. The current
Clang shared library is 95,128 B versus 135,376 B for `d8b7140` (-29.7%; identical
command and flags). The binary-size tradeoff is specific to Wasm: native code and
data are smaller while the generated Wasm data tables are larger.

## Allocation and peak-heap comparison

The Linux allocator probe was rerun against the immutable static-table candidate
and archived `d8b7140` using fresh processes, 1 MiB inputs, and one measured
conversion. Cold means no warmup; warm means five conversions before the probe
reset. Requested-byte totals sum malloc/calloc/realloc request sizes. The full
live/peak figures are tracked requested bytes, excluding allocator overhead.
No tracked-allocation overflow occurred and output checksums matched. The candidate
is the current packed-node source; that layout change does not affect heap behavior.

| Workload | Cold requests d8 → current | Cold requested bytes d8 → current | Cold peak live bytes d8 → current | Warm requests d8 → current |
|---|---:|---:|---:|---:|
| Devanagari → Telugu | 2,762 → 3 | 27,478,638 → 18,876,024 | 26,359,260 → 17,827,356 | 9 → 3 |
| Latin → Devanagari | 3,022 → 5 | 34,925,220 → 34,701,286 | 29,512,781 → 29,360,748 | 11 → 5 |
| Expansion-heavy | 2,763 → 4 | 30,622,237 → 22,020,268 | 27,405,773 → 18,874,514 | 10 → 4 |
| Protected spans | 2,762 → 3 | 27,417,794 → 18,815,708 | 26,357,560 → 17,826,184 | 9 → 3 |
| Mixed protected spans | 2,762 → 3 | 27,443,064 → 18,840,450 | 26,359,235 → 17,827,331 | 9 → 3 |
| Virtual Indic → ISO | 6,712 → 3 | 27,113,706 → 18,357,096 | 26,484,604 → 17,827,356 | 9 → 3 |

Separate Valgrind Massif runs measured peak `mem_heap_B` reductions of 29.9%
for Devanagari, 0.5% for Latin, 28.8% for expansion-heavy, 29.9% for protected,
29.9% for mixed protected, and 30.2% for virtual Indic input. These memory and
throughput gains are practical benefits of static lookup; they do not yet justify
the compressed Wasm increase under the plan's +5% acceptance limit.

## Current validation

On the current exact-root-interned source, `make test`, CMake Release/CTest,
ASan/UBSan with leak checking disabled, direct Dart SDK analysis, release
verification, deterministic regeneration of all three generated headers, and Node
Wasm smoke conversions pass. Output hashes matched in current benchmark comparisons.
The full 1,048,971-case option/output differential ran before exact root interning
(after the direct second-page lookup change); current native fixtures and full
benchmark output hashes cover the final representation. Browser fixtures could not
run because this sandbox denies starting a local HTTP server. Flutter tests were not
rerun because the Flutter launcher cannot write its SDK cache. `yarn test` was
previously blocked by ts-jest incompatibility with installed TypeScript 7.0.2.
LeakSanitizer cannot run under the sandbox's ptrace restriction. Apple/MSVC/plugin
hosted checks remain pending; no push was made to trigger CI.

## Immediate next actions

1. Reduce the generated Wasm data footprint while keeping the current measured
   throughput. The +5% compressed-size gate remains the merge blocker.
2. Repeat long-input pairs on a quieter machine and rerun the native differential
   on the final source after the remaining size work.
3. Hosted Apple/MSVC/plugin checks, Windows test execution (no Wine), Android
   runtime (adb unavailable), and LeakSanitizer (sandbox ptrace restriction) remain
   unverified locally. Do not push to trigger hosted CI. Do not retry the rejected
   Flutter pub publish dry-run.

## Historical validation

Earlier source revisions passed native tests, CMake/CTest on Linux, Dart analysis,
Flutter and Node tests, browser fixtures, ASan/UBSan, generator determinism,
49,152-case output differential, MinGW and Android arm64 cross-builds, Wasm builds,
and package dry-run checks. Consult `cpp23-performance.md` for exact scopes and
limitations; these results do not verify the current source unless rerun.
