# inditrans Benchmark & Performance Reference

This guide explains how to use the benchmark (`bench-*`) commands, snapshot (`perf-*`) tooling, and the automated runner (`tool/run-perf.sh`) for:

- quick performance checks during development;
- direct A/B regression testing against a baseline;
- comparing branches, tags, or historical commits;
- tracking incremental optimization milestones;
- automating multi-tag snapshot capture across Git history;
- producing cumulative performance reports.

## Mental model

Use this distinction:

> **`bench-*` = measure and regression-test code**  
> **`perf-*` = capture and report performance history**  
> **`tool/run-perf.sh` = automate multi-tag snapshot capture and report generation**

The main workflows are:

```text
quick measurement
    ↓
bench / bench-short / bench-cold / bench-all

direct A/B baseline comparison
    ↓
bench-save → make changes → bench-compare

manual optimization history
    ↓
perf-snapshot → perf-snapshot → ... → perf-report

automated milestone benchmark across Git tags
    ↓
git tag perf-* → tool/run-perf.sh [--current] → out/report.md
```

---

# 1. Quick reference

| Command | Purpose |
|---|---|
| `make bench` | Throughput benchmark |
| `make bench-short` | Short-call latency |
| `make bench-short-repeat` | Repeat short-call latency several times (`PERF_REPEATS=5`) |
| `make bench-cold` | Cold-start latency in fresh processes (`COLD_BENCH_SAMPLES=501`) |
| `make bench-lookup` | Static reader lookup microbenchmark |
| `make bench-lookup-alloc-linux` | Allocation check for lookup layer |
| `make bench-alloc-linux` | Whole-call allocation measurement (Linux/glibc) |
| `make output-size-bench` | Output expansion measurement |
| `make bench-all` | Human-readable summary of the major benchmarks |
| `make bench-save` | Save throughput, latency, and allocation data as an A/B baseline |
| `make bench-compare` | Compare current checkout against a saved baseline |
| `make perf-snapshot` | Save a comprehensive performance snapshot under `out/perf/` |
| `make perf-report` | Compare a sequence of saved snapshots against a baseline |
| `tool/run-perf.sh` | Automate snapshots for all `perf-*` tags and generate `out/report.md` |
| `tool/run-perf.sh --current` | Run all `perf-*` tags plus current commit as `99-current` |

---

# 2. Important variables

## Repeated benchmark runs

```bash
BENCH_RUNS=5
```

Default: `5`.

Used by:
- `bench-save`
- `bench-compare`
- `perf-snapshot`

It controls the number of complete throughput and short-latency benchmark runs.

Example:

```bash
make perf-snapshot PERF_NAME=00-baseline BENCH_RUNS=10
```

This runs throughput 10 times and short-call latency 10 times, saving both the aggregated averages and every raw run in CSV files.

---

## CPU pinning

```bash
BENCH_CPU=<cpu>
PERF_CPU=<cpu>
```

For reproducible measurements on Linux, pin benchmark processes to one specific CPU core.

Example:

```bash
CPU=$(python3 - <<'PY'
import os
print(min(os.sched_getaffinity(0)))
PY
)

make perf-snapshot \
    PERF_NAME=00-baseline \
    BENCH_CPU="$CPU" \
    PERF_CPU="$CPU"
```

- `BENCH_CPU` is used by repeated throughput and latency measurements.
- `PERF_CPU` is used by cold-start and other performance probes.

For serious comparisons, always pass the same CPU core to both.

---

## Cold-start samples

```bash
COLD_BENCH_SAMPLES=501
```

Default: `501`.

Each cold-start case is executed in that many fresh processes to measure process launch and initialization overhead. The value must be an odd number and at least 3.

There are currently four representative cold-start cases:
- `cold-devanagari-to-telugu`
- `cold-iso-to-devanagari`
- `cold-devanagari-to-tamil`
- `cold-indic-to-iso`

Example:

```bash
make bench-cold COLD_BENCH_SAMPLES=1001 PERF_CPU="$CPU"
```

---

## Repeated short-latency runs

```bash
PERF_REPEATS=5
```

Default: `5`.

Used by `bench-short-repeat`. Must be an odd number of at least 3. It runs multiple complete short-call latency sweeps to report stable median/IQR numbers.

---

## Snapshot directory

```bash
PERF_DIR=out/perf
```

Default: `out/perf`.

All `perf-snapshot` data and artifact files are stored here.

---

# 3. Quick development measurements

While actively modifying implementation code, use individual benchmark commands for rapid feedback.

## Throughput

```bash
make bench
```

Use this when optimizing sustained transliteration throughput over large text payloads.

---

## Short-call latency

```bash
make bench-short
```

Measures per-call transliteration latency for small strings, including output buffer destruction overhead.

For a more stable measurement:

```bash
make bench-short-repeat PERF_REPEATS=10
```

---

## Cold-start latency

```bash
make bench-cold
```

For higher statistical confidence:

```bash
make bench-cold COLD_BENCH_SAMPLES=1001
```

Measures the first transliteration call in a fresh process. Essential when modifying initialization, static tables, metadata loading, or lookup structures.

---

## Reader lookup performance

```bash
make bench-lookup
```

Isolated microbenchmark of reader lookup and trie operations.

On Linux/glibc:

```bash
make bench-lookup-alloc-linux
```

Verifies that the lookup layer remains strictly zero-allocation.

---

## Allocations

```bash
make bench-alloc-linux \
    ALLOC_BENCH_MODE=devanagari \
    ALLOC_BENCH_BYTES=1048576 \
    ALLOC_BENCH_REPETITIONS=1 \
    ALLOC_BENCH_WARMUPS=0
```

Allocation probing requires Linux with glibc. Measures malloc/calloc/realloc counts, total requested allocation bytes, peak requested live bytes, and post-call live bytes.

---

## Output expansion

```bash
make output-size-bench
```

Measures output string expansion and sizing across scripts.

---

## Everything at a glance

```bash
make bench-all
```

Prints a formatted terminal summary covering throughput, short-call latency, cold-start latency, output expansion, cold allocations, and warm allocations. Useful for a quick sanity check before committing.

---

# 4. `bench-save` and `bench-compare`

These commands are primarily for **direct A/B regression testing**.

## Save baseline

```bash
make bench-save
```

Default output prefix: `out/bench-baseline`.

Produces:

```text
out/bench-baseline-throughput.csv
out/bench-baseline-throughput-runs.csv
out/bench-baseline-latency.csv
out/bench-baseline-latency-runs.csv
out/bench-baseline-allocs.csv
out/bench-baseline-allocs-warm.csv
out/bench-baseline-info.txt
```

### What `bench-save` measures

Captures:
- throughput × `BENCH_RUNS`
- short-call latency × `BENCH_RUNS`
- cold allocations (Linux/glibc)
- warm allocations (Linux/glibc)
- system and git metadata

It does **not** capture:
- cold-start process latency
- Wasm artifact size
- reader lookup microbenchmarks
- output expansion

---

## Compare current code against that baseline

After making code changes:

```bash
make bench-compare
```

Compares the current checkout against `out/bench-baseline-*`. By default, regressions are reported without failing the command.

To enforce a strict regression gate:

```bash
make bench-compare \
    BENCH_REGRESSION_THRESHOLD=5 \
    BENCH_STRICT=1
```

This fails if any timing regresses by 5% or more. Output hash mismatches and missing benchmark cases always fail regardless of `BENCH_STRICT`.

---

# 5. Recommended simple baseline workflow

Use this when working locally on one branch and testing an incremental edit.

### Step 1 — Save the starting point

```bash
make bench-save \
    BENCH_BASELINE=out/baselines/start \
    BENCH_RUNS=5
```

### Step 2 — Make your implementation change

Edit code and rebuild.

### Step 3 — Compare

```bash
make bench-compare \
    BENCH_BASELINE=out/baselines/start \
    BENCH_RUNS=5
```

For high-precision comparisons on Linux, pin to the same CPU:

```bash
make bench-save \
    BENCH_BASELINE=out/baselines/start \
    BENCH_RUNS=10 \
    BENCH_CPU="$CPU"

make bench-compare \
    BENCH_BASELINE=out/baselines/start \
    BENCH_RUNS=10 \
    BENCH_CPU="$CPU"
```

---

# 6. Comparing two branches

## Method A — Simple branch switching

Use this when the benchmark harness itself has not changed between branches.

### Save `main`

```bash
git switch main

make bench-save \
    BENCH_BASELINE=out/baselines/main \
    BENCH_RUNS=5
```

### Switch and compare

```bash
git switch dev

make bench-compare \
    BENCH_BASELINE=out/baselines/main \
    BENCH_RUNS=5
```

### Compare two experimental branches

```bash
git switch experiment-a
make bench-save \
    BENCH_BASELINE=out/baselines/experiment-a \
    BENCH_RUNS=5

git switch experiment-b
make bench-compare \
    BENCH_BASELINE=out/baselines/experiment-a \
    BENCH_RUNS=5
```

---

# 7. Robust branch comparison with Git worktrees

For cleaner, more reliable A/B testing, use separate worktrees to avoid repeatedly checking out different branches in the same workspace.

### Worktrees for A/B regression (`bench-save` / `bench-compare`)

```bash
git worktree add /tmp/inditrans-main main
git worktree add /tmp/inditrans-dev dev

CPU=$(python3 - <<'PY'
import os
print(min(os.sched_getaffinity(0)))
PY
)

# 1. Save baseline in main
cd /tmp/inditrans-main
make bench-save \
    BENCH_BASELINE=/tmp/inditrans-main-vs-dev \
    BENCH_RUNS=10 \
    BENCH_CPU="$CPU"

# 2. Compare in dev
cd /tmp/inditrans-dev
make bench-compare \
    BENCH_BASELINE=/tmp/inditrans-main-vs-dev \
    BENCH_RUNS=10 \
    BENCH_CPU="$CPU"

# 3. Cleanup
git worktree remove /tmp/inditrans-main
git worktree remove /tmp/inditrans-dev
```

### Worktrees for snapshots (`perf-snapshot` / `perf-report`)

You can also use worktrees with a shared snapshot directory:

```bash
git worktree add /tmp/inditrans-base main
git worktree add /tmp/inditrans-test dev

# Capture baseline
cd /tmp/inditrans-base
make perf-snapshot \
    PERF_DIR=/tmp/inditrans-perf \
    PERF_NAME=00-main \
    BENCH_RUNS=5 \
    BENCH_CPU="$CPU" \
    PERF_CPU="$CPU"

# Capture test revision
cd /tmp/inditrans-test
make perf-snapshot \
    PERF_DIR=/tmp/inditrans-perf \
    PERF_NAME=10-dev \
    BENCH_RUNS=5 \
    BENCH_CPU="$CPU" \
    PERF_CPU="$CPU"

# Generate report
make perf-report \
    PERF_DIR=/tmp/inditrans-perf \
    PERF_BASELINE=00-main

# Cleanup
git worktree remove /tmp/inditrans-base
git worktree remove /tmp/inditrans-test
```

---

# 8. Important branch-comparison caveat

If the benchmark harness itself changed between branches, running each branch's own Makefile and benchmark binaries means the measurement system is no longer identical.

For serious regression testing across such revisions, use:

> **same benchmark harness + different engine revisions**

The CI performance job in `.github/workflows/` already implements this:
1. checks out the historical base revision in a worktree;
2. copies the current `native/bench` harness into it;
3. uses the current benchmark Makefile and tooling;
4. builds the historical engine using its checked-in `script_data.h`;
5. measures the current revision using the identical harness.

For day-to-day development where the benchmark suite has not changed, standard branch switching or worktrees are completely sufficient.

---

# 9. `perf-snapshot`

Use `perf-snapshot` when you want to preserve the **comprehensive performance state of an optimization milestone**.

```bash
make perf-snapshot PERF_NAME=00-baseline
```

Internally, this executes `bench-save`, and additionally captures:
- cold-start process latency across `COLD_BENCH_SAMPLES` runs;
- standalone Wasm artifact size (force-rebuilding `flutter/assets/inditrans.wasm`).

Structure of a snapshot:

```text
perf-snapshot
│
├── throughput × BENCH_RUNS
├── short-call latency × BENCH_RUNS
├── cold allocations (Linux/glibc)
├── warm allocations (Linux/glibc)
├── cold-start latency × COLD_BENCH_SAMPLES
├── Git & environment metadata
└── Wasm binary byte size
```

---

# 10. Files created by `perf-snapshot`

Running `make perf-snapshot PERF_NAME=00-baseline` creates under `out/perf/`:

```text
out/perf/00-baseline-throughput.csv
out/perf/00-baseline-throughput-runs.csv
out/perf/00-baseline-latency.csv
out/perf/00-baseline-latency-runs.csv
out/perf/00-baseline-allocs.csv
out/perf/00-baseline-allocs-warm.csv
out/perf/00-baseline-cold.csv
out/perf/00-baseline-wasm-size.txt
out/perf/00-baseline-info.txt
```

---

# 11. Recommended incremental optimization workflow

This is the primary workflow for multi-step tuning:

### 1. Capture reference implementation

```bash
make perf-snapshot PERF_NAME=00-baseline BENCH_RUNS=10
```

### 2. Implement optimization step 1 and capture

```bash
make perf-snapshot PERF_NAME=10-static-reader BENCH_RUNS=10
```

### 3. Implement optimization step 2 and capture

```bash
make perf-snapshot PERF_NAME=20-static-writer BENCH_RUNS=10
```

### 4. Implement optimization step 3 and capture

```bash
make perf-snapshot PERF_NAME=30-prefix-lookup BENCH_RUNS=10
```

### 5. Generate cumulative report

```bash
make perf-report
```

---

# 12. Snapshot naming convention

Always prefix snapshot names with a two-digit zero-padded number:

```text
00-baseline
10-static-reader
20-static-writer
30-prefix-lookup
40-final
```

`perf-report` automatically discovers all snapshots in `out/perf/` and sorts them lexicographically. Avoid names like `step1`, `step10`, `step2` where alphabetical sorting breaks chronological sequence.

---

# 13. `perf-report`

Generate the comparative report:

```bash
make perf-report
```

By default, it uses `00-baseline` as the reference:

```bash
make perf-report PERF_BASELINE=00-baseline
```

The report compares every discovered snapshot against the baseline across key metrics.

## What the report summarizes

By default, the report highlights representative cases:
- **Short latency**: `indic-to-indic`
- **Allocations**: `devanagari`
- **Cold start**: `cold-devanagari-to-telugu`
- **Wasm size**: `inditrans.wasm` byte size and delta
- **Metadata**: commit hash, branch, compiler

You can customize which representative cases are featured in the summary:

```bash
make perf-report \
    PERF_LATENCY_CASE=latin-input \
    PERF_ALLOC_CASE=latin \
    PERF_COLD_CASE=cold-iso-to-devanagari
```

All underlying CSV files preserve the full metrics for all benchmark cases.

---

# 14. Explicit snapshot ordering

If snapshot names do not sort naturally, use `tool/perf_report.py` directly with repeated `--step` flags:

```bash
python3 tool/perf_report.py \
    --dir out/perf \
    --baseline 00-baseline \
    --step 10-static-reader \
    --step 30-prefix-lookup \
    --step 20-static-writer
```

---

# 15. Comparing branches with full `perf-snapshot` data

```bash
git switch main
make perf-snapshot PERF_NAME=00-main BENCH_RUNS=10

git switch dev
make perf-snapshot PERF_NAME=10-dev BENCH_RUNS=10

make perf-report PERF_BASELINE=00-main
```

---

# 16. Comparing historical commits and Git tags

You can benchmark any Git tag or commit using a detached checkout:

### Comparing a release tag

```bash
git switch --detach v1.4.0

make perf-snapshot \
    PERF_NAME=00-v1.4.0 \
    BENCH_RUNS=5

# Return to active branch
git switch dev

make perf-snapshot \
    PERF_NAME=10-dev \
    BENCH_RUNS=5

make perf-report PERF_BASELINE=00-v1.4.0
```

### Comparing a specific commit hash

```bash
git switch --detach 126d17e

make perf-snapshot \
    PERF_NAME=00-reference \
    BENCH_RUNS=5

git switch dev

make perf-snapshot \
    PERF_NAME=10-current \
    BENCH_RUNS=5

make perf-report PERF_BASELINE=00-reference
```

Any valid Git ref works with `git switch --detach <ref>` (tag, commit SHA, or remote branch). To return from detached mode, run `git switch dev` (or `git switch -`).

---

# 17. Automated multi-snapshot runner: `tool/run-perf.sh`

When tracking long-term optimization milestones, manually checking out tags, capturing snapshots, and generating reports is repetitive. The repository provides an automated runner: [`tool/run-perf.sh`](file:///home/shasak/ws/inditrans/tool/run-perf.sh).

## Overview & capabilities

[`tool/run-perf.sh`](file:///home/shasak/ws/inditrans/tool/run-perf.sh) automates running snapshots across all tagged performance milestones and generating a Markdown report:

```text
git tag perf-*
      │
      ▼
tool/run-perf.sh [--current]
      │
      ├── Workspace hygiene: stashes dirty/untracked files
      ├── Signal trap: restores branch and stash on EXIT / INT / TERM
      ├── Iterates all Git tags matching 'perf-*'
      │     └── Incremental caching: skips tags already having wasm-size.txt
      ├── Optional '--current': benchmarks current HEAD as '99-current'
      └── Automated report: make perf-report > out/report.md
```

### Key features

1. **Workspace safety & automatic restoration**:
   - Detects dirty or untracked files (`git status --porcelain --untracked-files=all`).
   - Stashes them including untracked files (`git stash push --include-untracked -m "Before performance benchmarks"`).
   - Installs traps on `EXIT`, `INT` (Ctrl+C), and `TERM` (`restore_workspace`).
   - On completion or early interruption, it restores `flutter/assets/inditrans.wasm`, switches back to your original branch or commit, and cleanly reapplies and drops the stash.

2. **Incremental snapshot discovery & caching**:
   - Discovers all Git tags matching `perf-*` in sorted order:
     ```bash
     git tag --list 'perf-*' | LC_ALL=C sort
     ```
   - Strips the `perf-` prefix to determine the snapshot name (e.g., `perf-00-baseline` → `00-baseline`).
   - Checks if `out/perf/$name-wasm-size.txt` already exists. If present, it prints `Skipping $name` and skips the snapshot. This makes re-running the script incremental and very fast.

3. **Deterministic defaults**:
   - Pins both `BENCH_CPU` and `PERF_CPU` to core `0` (`CPU=0`).
   - Sets `BENCH_RUNS` to `5` (`RUNS=5`).

4. **`--current` flag**:
   When passed `--current`, after evaluating all tagged milestones, it also captures the current checkout commit as snapshot `99-current`.

5. **Automatic report generation**:
   At the end of the run, it automatically executes:
   ```bash
   make perf-report PERF_BASELINE=00-baseline > out/report.md
   ```
   writing the full Markdown comparison table directly to `out/report.md`.

## Usage

### Run all tagged milestones

```bash
tool/run-perf.sh
```

### Run tagged milestones plus current working tree

```bash
tool/run-perf.sh --current
```

### Tagging workflow for `run-perf.sh`

To add a milestone to the automated suite, tag any commit with the `perf-<name>` naming scheme:

```bash
git tag perf-00-baseline <commit>
git tag perf-01-buffer-ownership <commit>
git tag perf-02-tamil-lookup <commit>
git tag perf-03-stack-reader-writer <commit>
```

When `tool/run-perf.sh` runs, it will automatically detect these tags, create snapshots named `00-baseline`, `01-buffer-ownership`, etc., skip any previously cached snapshots, and compile `out/report.md`.

---

# 18. Baseline versus incremental snapshots

There are two primary ways to interpret snapshot comparisons:

## Cumulative improvement

```text
baseline → step 1
baseline → step 2
baseline → step 3
```

This is the default view in `perf-report`. It answers:
> *“How much faster, smaller, or leaner is the engine compared to the starting point?”*

Example:

```text
00-baseline        1.25 µs     0 B live
10-static-reader   -5.2%       0 B live
20-static-writer   -12.8%      0 B live
30-prefix-lookup   -18.4%      0 B live
```

---

## Incremental improvement

```text
step 1 → step 2
step 2 → step 3
```

To see the isolated impact of later optimizations relative to an intermediate step, override the baseline:

```bash
make perf-report PERF_BASELINE=20-static-writer
```

Now all subsequent snapshots are compared directly against `20-static-writer`.

---

# 19. Common recipes

### Recipe 1: `main` vs `dev`

```bash
git switch main
make perf-snapshot PERF_NAME=00-main BENCH_RUNS=5

git switch dev
make perf-snapshot PERF_NAME=10-dev BENCH_RUNS=5

make perf-report PERF_BASELINE=00-main
```

### Recipe 2: Release tag vs current branch

```bash
git switch --detach v1.4.0
make perf-snapshot PERF_NAME=00-v1.4.0 BENCH_RUNS=5

git switch dev
make perf-snapshot PERF_NAME=10-dev BENCH_RUNS=5

make perf-report PERF_BASELINE=00-v1.4.0
```

### Recipe 3: Historical commit vs current branch

```bash
git switch --detach 126d17e
make perf-snapshot PERF_NAME=00-reference BENCH_RUNS=5

git switch dev
make perf-snapshot PERF_NAME=10-current BENCH_RUNS=5

make perf-report PERF_BASELINE=00-reference
```

### Recipe 4: Incremental tuning sequence

```bash
make perf-snapshot PERF_NAME=00-baseline BENCH_RUNS=5

# apply change 1
make perf-snapshot PERF_NAME=10-step1 BENCH_RUNS=5

# apply change 2
make perf-snapshot PERF_NAME=20-step2 BENCH_RUNS=5

# apply change 3
make perf-snapshot PERF_NAME=30-step3 BENCH_RUNS=5

make perf-report
```

### Recipe 5: Automated suite across Git tags

```bash
tool/run-perf.sh --current
cat out/report.md
```

---

# 20. Recommended workflow for optimization experiments

```text
main / known-good reference
         │
         ▼
00-baseline
         │
         ├── optimization A
         ▼
10-static-reader
         │
         ├── optimization B
         ▼
20-static-writer
         │
         ├── optimization C
         ▼
30-prefix-lookup
```

At each meaningful stage:

```bash
make perf-snapshot PERF_NAME=<number>-<description>
```

Then:

```bash
make perf-report
```

If one step looks suspicious, isolate it:

```bash
make perf-report PERF_BASELINE=20-static-writer
```

---

# 21. Recommended settings for serious measurements

For quick interactive development:

```bash
BENCH_RUNS=5
COLD_BENCH_SAMPLES=501
```

For definitive before/after evaluation:

```bash
BENCH_RUNS=10
COLD_BENCH_SAMPLES=501
```

For high-confidence publication or release benchmarking:

```bash
BENCH_RUNS=20
COLD_BENCH_SAMPLES=1001
```

Pin both revisions to the same CPU core on Linux:

```bash
CPU=$(python3 - <<'PY'
import os
print(min(os.sched_getaffinity(0)))
PY
)

make perf-snapshot \
    PERF_NAME=00-baseline \
    BENCH_RUNS=10 \
    BENCH_CPU="$CPU" \
    PERF_CPU="$CPU"
```

Use identical settings for every snapshot in the series.

---

# 22. Interpreting repeated runs

`bench-save` and `perf-snapshot` generate two tiers of CSV files:
- Individual raw runs: `*-runs.csv`
- Aggregated summaries: `*.csv`

`repeat_short_bench.py` executes the entire benchmark `BENCH_RUNS` times. The aggregate timing metric is the arithmetic mean across runs.

It also validates that:
- output byte length (`output_size_sink`) remains identical;
- FNV-1a output hash (`output_fnv1a64`) matches across all runs.

If benchmark output changes between runs, the benchmark capture aborts immediately.

---

# 23. Output correctness checks

Performance tuning must never alter transliteration output unless explicitly intended.

Throughput and latency CSVs include:
```text
output_size_sink
output_fnv1a64
```

`bench-compare` validates these hashes against the baseline. Any implementation that runs faster but produces mismatched output will immediately fail the comparison, making it safe for automated CI regression gating.

---

# 24. Allocation measurements

Allocation metrics are measured via glibc memory interception on Linux. On other platforms, allocation probes are skipped automatically.

The probe captures:
- total allocation events (`malloc + calloc + realloc`);
- requested allocation bytes;
- peak requested live heap bytes;
- live requested heap bytes remaining after execution.

Two separate measurements are taken:
- `*-allocs.csv`: Cold allocation behavior (first invocation, no warmup).
- `*-allocs-warm.csv`: Warm allocation behavior (after one unmeasured transliteration).

---

# 25. Wasm size

`perf-snapshot` force-rebuilds `flutter/assets/inditrans.wasm` and logs its raw byte size. Wasm size is tracked as diagnostic tradeoff data rather than an automated pass/fail gate. It ensures runtime speedups do not quietly inflate binary distribution size.

---

# 26. What `perf-snapshot` currently does NOT include

`perf-snapshot` covers the primary engine transliteration paths, but currently does not save:
- `bench-lookup` (microbenchmark for reader trie lookup);
- `bench-lookup-alloc-linux` (allocation probe for reader trie);
- `output-size-bench` (output expansion ratios).

Run these commands separately when modifying those specific subsystems.

---

# 27. Which command should I use?

| Scenario | Command |
|---|---|
| *“I changed a small function and want to know if it got faster.”* | `make bench` or `make bench-short` |
| *“I changed trie/reader lookup logic.”* | `make bench-lookup` and `make bench-lookup-alloc-linux` |
| *“I changed metadata or static initialization.”* | `make bench-cold` and `make bench-alloc-linux` |
| *“I want a broad health check across all benchmarks right now.”* | `make bench-all` |
| *“I want to compare this edit directly against where I started.”* | `make bench-save` then `make bench-compare` |
| *“I am doing a multi-step optimization milestone.”* | `make perf-snapshot PERF_NAME=<nn>-<name>` then `make perf-report` |
| *“I want to run and compare all historical git tags + my current commit.”* | `tool/run-perf.sh --current` |
| *“I want to compare main with dev.”* | `git switch main; make perf-snapshot ...; git switch dev; make perf-snapshot ...; make perf-report` |

---

# 28. Recommended directory convention

Keep baselines and tuning histories organized:

```text
out/baselines/    # Direct A/B regression baselines (bench-save / bench-compare)
out/perf/         # Cumulative tuning snapshots (perf-snapshot)
out/report.md     # Automated report from tool/run-perf.sh
```

---

# 29. Suggested standard workflow for inditrans

For major performance PRs, follow this sequence:

1. **Establish baseline**:
   ```bash
   make perf-snapshot PERF_NAME=00-baseline BENCH_RUNS=10 BENCH_CPU=0 PERF_CPU=0
   ```
2. **Develop with fast targeted benchmarks**:
   ```bash
   make bench-short
   make bench-cold
   ```
3. **Capture milestone snapshot**:
   ```bash
   make perf-snapshot PERF_NAME=10-my-optimization BENCH_RUNS=10 BENCH_CPU=0 PERF_CPU=0
   ```
4. **Tag milestone**:
   ```bash
   git tag perf-10-my-optimization
   ```
5. **Review cumulative progress**:
   ```bash
   make perf-report
   # Or run automated suite:
   tool/run-perf.sh --current
   ```
6. **Run strict regression check**:
   ```bash
   make bench-compare \
       BENCH_BASELINE=out/baselines/reference \
       BENCH_RUNS=10 \
       BENCH_CPU=0 \
       BENCH_REGRESSION_THRESHOLD=5 \
       BENCH_STRICT=1
   ```

---

# 30. Cheat sheet

```bash
# Fast individual checks
make bench
make bench-short
make bench-short-repeat PERF_REPEATS=10
make bench-cold
make bench-lookup
make bench-all

# Direct A/B regression
make bench-save BENCH_BASELINE=out/baselines/base BENCH_RUNS=10
make bench-compare BENCH_BASELINE=out/baselines/base BENCH_RUNS=10

# Snapshot management
make perf-snapshot PERF_NAME=00-baseline BENCH_RUNS=10
make perf-snapshot PERF_NAME=10-step-one BENCH_RUNS=10
make perf-snapshot PERF_NAME=20-step-two BENCH_RUNS=10

# Generate comparison report
make perf-report
make perf-report PERF_BASELINE=10-step-one

# Automated tag-based runner
tool/run-perf.sh
tool/run-perf.sh --current
```

---

# 31. Summary & Rules of thumb

```text
bench-*           while actively developing
bench-save        before an A/B experiment
bench-compare     after an A/B experiment
perf-snapshot     at meaningful optimization milestones
perf-report       to review the full optimization progression
tool/run-perf.sh  to automate multi-tag snapshot benchmarking and reports
```

### Rule of thumb

```text
perf-snapshot
    = capture one revision / optimization step

perf-report
    = compare captured snapshots
```

For reliable, reproducible comparisons across snapshots, keep these identical:
- `BENCH_RUNS`
- `BENCH_CPU` / `PERF_CPU`
- `COLD_BENCH_SAMPLES`
- Compiler and toolchain version
- Benchmark harness code