#!/usr/bin/env python3
"""Paired baseline/candidate benchmark runner with counterbalanced execution order.

Isolates benchmarks against host-system noise (WSL2 CPU scheduling, background load,
and thermal changes) by interleaving baseline and candidate executions close together
in time using counterbalanced alternating order (AB / BA).

Calculates regression as the median of paired execution ratios (candidate_i / baseline_i).
"""

from __future__ import annotations

import argparse
import atexit
import csv
import glob
import os
import shutil
import signal
import subprocess
import sys
import tempfile
from dataclasses import dataclass
from pathlib import Path
from typing import Sequence

ROOT = Path(__file__).resolve().parent.parent
if str(ROOT / "tool") not in sys.path:
    sys.path.insert(0, str(ROOT / "tool"))

from perf_stats import (
    classify_quality,
    compute_mad,
    compute_relative_mad,
    median_paired_ratio,
    paired_regression_pct,
    rounded_median,
)


@dataclass
class RunRecord:
    round_index: int
    execution_order: int
    target: str  # "baseline" or "candidate"
    benchmark: str  # "latency" or "throughput"
    case: str
    input_bytes: int
    primary_ns: int
    p95_ns: int
    output_size_sink: int
    output_fnv1a64: str


@dataclass
class PairedStat:
    benchmark: str
    case: str
    input_bytes: int
    rounds: int
    baseline_median_ns: int
    candidate_median_ns: int
    baseline_mad_ns: float
    baseline_rel_mad_pct: float
    candidate_mad_ns: float
    candidate_rel_mad_pct: float
    median_ratio: float
    regression_pct: float
    baseline_quality: str
    candidate_quality: str
    hash_match: bool


def parse_args(argv: Sequence[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--baseline", default="perf-00-baseline", help="baseline git ref or commit SHA (default: perf-00-baseline)")
    parser.add_argument("--candidate", default="HEAD", help="candidate git ref, commit SHA, or WORKSPACE (default: HEAD)")
    parser.add_argument("--runs", type=int, default=9, help="number of paired execution rounds (default: 9)")
    parser.add_argument("--cpu", default="", help="optional Linux CPU id passed to taskset (e.g. 0)")
    parser.add_argument("--suite", choices=["latency", "throughput", "all"], default="all", help="benchmark suite to run (default: all)")
    parser.add_argument("--output-dir", type=Path, default=Path("out/perf"), help="directory to store paired output files (default: out/perf)")
    parser.add_argument("--noise-warning-pct", type=float, default=2.0, help="relative MAD threshold for noisy warning (default: 2.0)")
    parser.add_argument("--noise-critical-pct", type=float, default=5.0, help="relative MAD threshold for critical noise (default: 5.0)")
    parser.add_argument("--fail-on-hash-mismatch", action=argparse.BooleanOptionalAction, default=True, help="fail if FNV-1a output hashes mismatch")
    return parser.parse_args(argv)


def get_git_commit(repo_root: Path, ref: str) -> str:
    try:
        proc = subprocess.run(
            ["git", "rev-parse", ref],
            cwd=repo_root,
            capture_output=True,
            text=True,
            check=True,
        )
        return proc.stdout.strip()
    except subprocess.CalledProcessError:
        return ref


def is_git_dirty(repo_root: Path) -> bool:
    try:
        proc = subprocess.run(
            ["git", "status", "--porcelain"],
            cwd=repo_root,
            capture_output=True,
            text=True,
            check=True,
        )
        return bool(proc.stdout.strip())
    except subprocess.CalledProcessError:
        return False


def build_binaries(
    repo_root: Path,
    ref: str,
    target_dir: Path,
    name: str,
) -> tuple[Path, Path]:
    """Extract and compile benchmark binaries for a given ref into target_dir."""
    src_dir = target_dir / "native" / "src"
    bench_dir = target_dir / "native" / "bench"
    src_dir.mkdir(parents=True, exist_ok=True)
    bench_dir.mkdir(parents=True, exist_ok=True)

    # If ref is WORKSPACE, copy working directory native/src directly
    if ref.upper() == "WORKSPACE" or (ref == "HEAD" and is_git_dirty(repo_root)):
        shutil.copytree(repo_root / "native" / "src", src_dir, dirs_exist_ok=True)
    else:
        # Extract native/src from git archive
        archive_proc = subprocess.Popen(
            ["git", "archive", ref, "native/src"],
            cwd=repo_root,
            stdout=subprocess.PIPE,
        )
        tar_proc = subprocess.run(
            ["tar", "-x", "-C", str(target_dir)],
            stdin=archive_proc.stdout,
            capture_output=True,
            check=True,
        )
        if archive_proc.stdout:
            archive_proc.stdout.close()
        archive_proc.wait()
        if archive_proc.returncode != 0:
            raise RuntimeError(f"git archive failed for ref {ref}: {tar_proc.stderr.decode('utf-8')}")

    # Copy current benchmark harness to ensure compatible measurement code
    shutil.copytree(repo_root / "native" / "bench", bench_dir, dirs_exist_ok=True)

    cpp_files = sorted(glob.glob(str(src_dir / "*.cpp")))
    if not cpp_files:
        raise RuntimeError(f"No C++ source files found in {src_dir} for {name} ({ref})")

    short_bench_bin = target_dir / f"{name}_short_bench"
    throughput_bench_bin = target_dir / f"{name}_throughput_bench"

    # Compile short_call
    cmd_short = [
        "clang++",
        "-std=c++23",
        "-O3",
        "-DNDEBUG",
        f"-I{src_dir}",
        *cpp_files,
        str(bench_dir / "short_call.cpp"),
        "-o",
        str(short_bench_bin),
    ]
    subprocess.run(cmd_short, check=True)

    # Compile benchmark
    cmd_throughput = [
        "clang++",
        "-std=c++23",
        "-O3",
        "-DNDEBUG",
        f"-I{src_dir}",
        *cpp_files,
        str(bench_dir / "benchmark.cpp"),
        "-o",
        str(throughput_bench_bin),
    ]
    subprocess.run(cmd_throughput, check=True)

    return short_bench_bin, throughput_bench_bin


def execute_benchmark(binary: Path, cpu: str) -> list[dict[str, str]]:
    cmd = [str(binary)]
    if cpu:
        cmd = ["taskset", "-c", cpu, *cmd]
    proc = subprocess.run(cmd, capture_output=True, text=True, check=True)
    reader = csv.DictReader(proc.stdout.splitlines())
    return list(reader)


def run_paired_benchmarks(
    repo_root: Path,
    baseline_ref: str,
    candidate_ref: str,
    runs: int,
    cpu: str,
    suite: str,
    output_dir: Path,
    noise_warning_pct: float,
    noise_critical_pct: float,
    fail_on_hash_mismatch: bool,
) -> tuple[int, list[PairedStat], list[RunRecord]]:
    if runs < 1:
        raise ValueError("Paired benchmark runs must be at least 1")

    temp_root = tempfile.mkdtemp(prefix="inditrans-paired-")
    temp_path = Path(temp_root)

    def cleanup():
        if temp_path.is_dir():
            shutil.rmtree(temp_path, ignore_errors=True)

    atexit.register(cleanup)

    def signal_handler(signum, frame):
        cleanup()
        sys.exit(128 + signum)

    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)

    print(f"==> Preparing baseline ({baseline_ref})...", flush=True)
    baseline_short, baseline_throughput = build_binaries(
        repo_root, baseline_ref, temp_path / "baseline", "baseline"
    )

    print(f"==> Preparing candidate ({candidate_ref})...", flush=True)
    candidate_short, candidate_throughput = build_binaries(
        repo_root, candidate_ref, temp_path / "candidate", "candidate"
    )

    suites_to_run = []
    if suite in ("latency", "all"):
        suites_to_run.append(("latency", baseline_short, candidate_short, "p50_ns", "p95_ns"))
    if suite in ("throughput", "all"):
        suites_to_run.append(("throughput", baseline_throughput, candidate_throughput, "median_ns", "sample_p95_ns"))

    raw_records: list[RunRecord] = []
    completed_rounds = 0

    print(f"==> Starting {runs} counterbalanced paired rounds (CPU: {cpu or 'unpinned'})...", flush=True)

    try:
        for r in range(1, runs + 1):
            if r % 2 == 1:
                order = [("baseline", 1), ("candidate", 2)]
            else:
                order = [("candidate", 1), ("baseline", 2)]

            round_records: list[RunRecord] = []

            for target_name, exec_pos in order:
                for b_name, b_bin, c_bin, prim_col, p95_col in suites_to_run:
                    target_bin = b_bin if target_name == "baseline" else c_bin
                    rows = execute_benchmark(target_bin, cpu)
                    for row in rows:
                        round_records.append(
                            RunRecord(
                                round_index=r,
                                execution_order=exec_pos,
                                target=target_name,
                                benchmark=b_name,
                                case=row["case"],
                                input_bytes=int(row["input_bytes"]),
                                primary_ns=int(float(row[prim_col])),
                                p95_ns=int(float(row[p95_col])),
                                output_size_sink=int(row["output_size_sink"]),
                                output_fnv1a64=row["output_fnv1a64"],
                            )
                        )

            raw_records.extend(round_records)
            completed_rounds += 1
            print(f"    Completed round {r}/{runs} ({order[0][0]} -> {order[1][0]})", flush=True)

    except (KeyboardInterrupt, subprocess.CalledProcessError) as exc:
        print(f"\nWarning: Paired benchmarking interrupted after {completed_rounds} complete rounds: {exc}", file=sys.stderr)

    if completed_rounds < 1:
        print("Error: No complete paired rounds were executed.", file=sys.stderr)
        return 1, [], []

    valid_records = [rec for rec in raw_records if rec.round_index <= completed_rounds]

    keys = sorted(list({(rec.benchmark, rec.case, rec.input_bytes) for rec in valid_records}))
    stats_list: list[PairedStat] = []
    hash_mismatches: list[str] = []

    for b_name, case_name, bytes_val in keys:
        b_records = [
            r for r in valid_records
            if r.benchmark == b_name and r.case == case_name and r.input_bytes == bytes_val and r.target == "baseline"
        ]
        c_records = [
            r for r in valid_records
            if r.benchmark == b_name and r.case == case_name and r.input_bytes == bytes_val and r.target == "candidate"
        ]

        b_records.sort(key=lambda r: r.round_index)
        c_records.sort(key=lambda r: r.round_index)

        b_timings = [float(r.primary_ns) for r in b_records]
        c_timings = [float(r.primary_ns) for r in c_records]

        b_hashes = {r.output_fnv1a64 for r in b_records}
        c_hashes = {r.output_fnv1a64 for r in c_records}
        hash_match = (b_hashes == c_hashes) and (len(b_hashes) == 1)

        if not hash_match:
            hash_mismatches.append(f"{case_name} ({bytes_val} B): baseline={b_hashes} candidate={c_hashes}")

        b_median = rounded_median(b_timings)
        c_median = rounded_median(c_timings)
        b_mad = compute_mad(b_timings)
        c_mad = compute_mad(c_timings)
        b_rel_mad = compute_relative_mad(b_timings)
        c_rel_mad = compute_relative_mad(c_timings)

        b_qual = classify_quality(b_rel_mad, noise_warning_pct, noise_critical_pct)
        c_qual = classify_quality(c_rel_mad, noise_warning_pct, noise_critical_pct)

        med_ratio = median_paired_ratio(b_timings, c_timings)
        regr_pct = paired_regression_pct(b_timings, c_timings)

        stats_list.append(
            PairedStat(
                benchmark=b_name,
                case=case_name,
                input_bytes=bytes_val,
                rounds=completed_rounds,
                baseline_median_ns=b_median,
                candidate_median_ns=c_median,
                baseline_mad_ns=b_mad,
                baseline_rel_mad_pct=b_rel_mad,
                candidate_mad_ns=c_mad,
                candidate_rel_mad_pct=c_rel_mad,
                median_ratio=med_ratio,
                regression_pct=regr_pct,
                baseline_quality=b_qual,
                candidate_quality=c_qual,
                hash_match=hash_match,
            )
        )

    output_dir.mkdir(parents=True, exist_ok=True)

    runs_csv_path = output_dir / "paired-runs.csv"
    with runs_csv_path.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.writer(handle)
        writer.writerow([
            "round",
            "execution_order",
            "target",
            "benchmark",
            "case",
            "input_bytes",
            "primary_ns",
            "p95_ns",
            "output_size_sink",
            "output_fnv1a64",
        ])
        for r in valid_records:
            writer.writerow([
                r.round_index,
                r.execution_order,
                r.target,
                r.benchmark,
                r.case,
                r.input_bytes,
                r.primary_ns,
                r.p95_ns,
                r.output_size_sink,
                r.output_fnv1a64,
            ])

    stats_csv_path = output_dir / "paired-stats.csv"
    with stats_csv_path.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.writer(handle)
        writer.writerow([
            "benchmark",
            "case",
            "input_bytes",
            "rounds",
            "baseline_median_ns",
            "candidate_median_ns",
            "baseline_mad_ns",
            "baseline_rel_mad_pct",
            "candidate_mad_ns",
            "candidate_rel_mad_pct",
            "median_ratio",
            "regression_pct",
            "baseline_quality",
            "candidate_quality",
            "hash_match",
        ])
        for s in stats_list:
            writer.writerow([
                s.benchmark,
                s.case,
                s.input_bytes,
                s.rounds,
                s.baseline_median_ns,
                s.candidate_median_ns,
                f"{s.baseline_mad_ns:.1f}",
                f"{s.baseline_rel_mad_pct:.2f}",
                f"{s.candidate_mad_ns:.1f}",
                f"{s.candidate_rel_mad_pct:.2f}",
                f"{s.median_ratio:.4f}",
                f"{s.regression_pct:.2f}",
                s.baseline_quality,
                s.candidate_quality,
                "1" if s.hash_match else "0",
            ])

    report_md_path = output_dir / "paired-report.md"
    report_content = generate_markdown_report(
        baseline_ref=baseline_ref,
        candidate_ref=candidate_ref,
        completed_rounds=completed_rounds,
        cpu=cpu,
        stats_list=stats_list,
        hash_mismatches=hash_mismatches,
        noise_warning_pct=noise_warning_pct,
        noise_critical_pct=noise_critical_pct,
    )
    report_md_path.write_text(report_content, encoding="utf-8")

    print(f"\nPaired benchmark report generated → {report_md_path}")
    print(f"Paired statistics saved         → {stats_csv_path}")
    print(f"Raw paired execution runs saved  → {runs_csv_path}")

    exit_code = 0
    if hash_mismatches:
        print(f"\nERROR: Output hash mismatches detected on {len(hash_mismatches)} cases!", file=sys.stderr)
        for mm in hash_mismatches:
            print(f"  - {mm}", file=sys.stderr)
        if fail_on_hash_mismatch:
            exit_code = 1

    return exit_code, stats_list, valid_records


def generate_markdown_report(
    baseline_ref: str,
    candidate_ref: str,
    completed_rounds: int,
    cpu: str,
    stats_list: list[PairedStat],
    hash_mismatches: list[str],
    noise_warning_pct: float,
    noise_critical_pct: float,
) -> str:
    lines = [
        "# Paired Performance Comparison Report",
        "",
        f"- **Baseline**: `{baseline_ref}`",
        f"- **Candidate**: `{candidate_ref}`",
        f"- **Paired Rounds**: {completed_rounds} (counterbalanced alternating AB / BA execution)",
        f"- **CPU Affinity**: `{cpu or 'unpinned'}`",
        "- **Metric Policy**: Median of round-level paired ratios (`candidate_i / baseline_i`)",
        "",
        "## Methodology",
        "",
        "To isolate performance comparisons from host-system background load, WSL2 CPU scheduling, "
        "and thermal variations, baseline and candidate are executed in close succession. "
        "The measurement order alternates between rounds (`A -> B` on odd rounds, `B -> A` on even rounds) "
        "to counterbalance systematic warmup or thermal ordering effects. "
        "Changes are computed as the median of paired ratios rather than the ratio of pooled aggregates, "
        "providing high breakdown-point resilience against temporary host hitches.",
        "",
    ]

    noisy_cases = [
        s for s in stats_list
        if s.baseline_rel_mad_pct > noise_warning_pct or s.candidate_rel_mad_pct > noise_warning_pct
    ]
    if noisy_cases:
        lines.extend([
            "> [!WARNING]",
            "> **Measurement Quality Warnings:**",
            f"> The following cases exhibited elevated cross-run variability (relative MAD > {noise_warning_pct:.1f}%):",
        ])
        for s in noisy_cases:
            parts = []
            if s.baseline_rel_mad_pct > noise_warning_pct:
                parts.append(f"baseline: {s.baseline_rel_mad_pct:.1f}% ({s.baseline_quality})")
            if s.candidate_rel_mad_pct > noise_warning_pct:
                parts.append(f"candidate: {s.candidate_rel_mad_pct:.1f}% ({s.candidate_quality})")
            lines.append(f"> - **{s.case}** ({s.input_bytes} B, {s.benchmark}): " + ", ".join(parts))
        lines.extend([
            "> ",
            "> Unstable small differences should be interpreted with caution. Repeating with additional rounds "
            "or dedicated core isolation (`--cpu <core>`) is recommended.",
            "",
        ])

    if hash_mismatches:
        lines.extend([
            "> [!CAUTION]",
            "> **Output Hash Mismatches Detected:**",
            "> The following cases produced mismatched FNV-1a output hashes between baseline and candidate:",
        ])
        for mm in hash_mismatches:
            lines.append(f"> - {mm}")
        lines.append("")

    latency_stats = [s for s in stats_list if s.benchmark == "latency"]
    if latency_stats:
        lines.extend([
            "## Short-Call Latency (Per-Call p50)",
            "",
            "| Case | Input | Baseline (p50) | Candidate (p50) | Paired Ratio | Change | Baseline MAD | Candidate MAD | Quality |",
            "|:---|---:|---:|---:|---:|---:|---:|---:|:---|",
        ])
        for s in latency_stats:
            change_str = f"{s.regression_pct:+.1f}%"
            qual_str = "stable" if s.baseline_quality == "stable" and s.candidate_quality == "stable" else f"{s.baseline_quality}/{s.candidate_quality}"
            lines.append(
                f"| {s.case} | {s.input_bytes} B | {s.baseline_median_ns} ns | {s.candidate_median_ns} ns | "
                f"{s.median_ratio:.4f} | {change_str} | {s.baseline_rel_mad_pct:.1f}% | {s.candidate_rel_mad_pct:.1f}% | {qual_str} |"
            )
        lines.append("")

    throughput_stats = [s for s in stats_list if s.benchmark == "throughput"]
    if throughput_stats:
        lines.extend([
            "## Throughput Latency (Per-Call Median)",
            "",
            "| Case | Input | Baseline (Median) | Candidate (Median) | Paired Ratio | Change | Baseline MAD | Candidate MAD | Quality |",
            "|:---|---:|---:|---:|---:|---:|---:|---:|:---|",
        ])
        for s in throughput_stats:
            change_str = f"{s.regression_pct:+.1f}%"
            qual_str = "stable" if s.baseline_quality == "stable" and s.candidate_quality == "stable" else f"{s.baseline_quality}/{s.candidate_quality}"
            lines.append(
                f"| {s.case} | {s.input_bytes} B | {s.baseline_median_ns} ns | {s.candidate_median_ns} ns | "
                f"{s.median_ratio:.4f} | {change_str} | {s.baseline_rel_mad_pct:.1f}% | {s.candidate_rel_mad_pct:.1f}% | {qual_str} |"
            )
        lines.append("")

    return "\n".join(lines) + "\n"


def main() -> int:
    args = parse_args()
    code, _, _ = run_paired_benchmarks(
        repo_root=ROOT,
        baseline_ref=args.baseline,
        candidate_ref=args.candidate,
        runs=args.runs,
        cpu=args.cpu,
        suite=args.suite,
        output_dir=args.output_dir,
        noise_warning_pct=args.noise_warning_pct,
        noise_critical_pct=args.noise_critical_pct,
        fail_on_hash_mismatch=args.fail_on_hash_mismatch,
    )
    return code


if __name__ == "__main__":
    raise SystemExit(main())
