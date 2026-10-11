#!/usr/bin/env python3
"""Run a native CSV benchmark repeatedly and emit aggregated timing values."""

from __future__ import annotations

import argparse
import csv
import shutil
import subprocess
import sys
from pathlib import Path

# Support running both as `tool/repeat_short_bench.py` and when sys.path lacks `tool/`
ROOT = Path(__file__).resolve().parent.parent
if str(ROOT / "tool") not in sys.path:
    sys.path.insert(0, str(ROOT / "tool"))

from perf_stats import (
    aggregate_metric,
    classify_quality,
    compute_mad,
    compute_relative_mad,
    rounded_mean,
    rounded_median,
)


THROUGHPUT_HEADER = [
    "case",
    "input_bytes",
    "median_ns",
    "sample_p95_ns",
    "output_size_sink",
    "output_fnv1a64",
]
LATENCY_HEADER = [
    "case",
    "input_bytes",
    "p50_ns",
    "p95_ns",
    "output_size_sink",
    "output_fnv1a64",
]
SCHEMAS = {
    tuple(THROUGHPUT_HEADER): ("median_ns", "sample_p95_ns"),
    tuple(LATENCY_HEADER): ("p50_ns", "p95_ns"),
}

STATS_HEADER = [
    "case",
    "input_bytes",
    "runs",
    "median_ns",
    "min_ns",
    "max_ns",
    "mad_ns",
    "relative_mad_pct",
    "p95_median_ns",
    "p95_min_ns",
    "p95_max_ns",
    "p95_mad_ns",
    "p95_relative_mad_pct",
    "quality",
]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, required=True, help="benchmark executable")
    parser.add_argument("--runs", type=int, default=5, help="number of complete benchmark runs")
    parser.add_argument("--cpu", default="", help="optional Linux CPU id/list passed to taskset")
    parser.add_argument(
        "--aggregate",
        choices=["median", "mean"],
        default="median",
        help="aggregation method across repeated runs (default: median)",
    )
    parser.add_argument(
        "--raw-output",
        type=Path,
        help="optional CSV path that receives every unaggregated run",
    )
    parser.add_argument(
        "--stats-output",
        type=Path,
        help="optional CSV path that receives variability and quality statistics",
    )
    parser.add_argument(
        "--noise-warning-pct",
        type=float,
        default=2.0,
        help="threshold percentage for classifying run as noisy (default: 2.0)",
    )
    parser.add_argument(
        "--noise-critical-pct",
        type=float,
        default=5.0,
        help="threshold percentage for classifying run as highly unstable (default: 5.0)",
    )
    return parser.parse_args()


def command(binary: Path, cpu: str) -> list[str]:
    cmd = [str(binary)]
    if not cpu:
        return cmd
    taskset = shutil.which("taskset")
    if taskset is None:
        raise ValueError("--cpu requires taskset")
    return [taskset, "-c", cpu, *cmd]


def main() -> int:
    args = parse_args()
    if args.runs < 1:
        print("repeat count must be at least 1", file=sys.stderr)
        return 2
    if not args.binary.is_file():
        print(f"benchmark executable does not exist: {args.binary}", file=sys.stderr)
        return 2

    header: list[str] | None = None
    metric_columns: tuple[str, str] | None = None
    order: list[tuple[str, str]] = []
    stable: dict[tuple[str, str], tuple[str, str]] = {}
    metrics: dict[tuple[str, str], tuple[list[float], list[float]]] = {}
    raw_rows: list[list[str]] = []

    try:
        for run_index in range(args.runs):
            sample = subprocess.run(
                command(args.binary, args.cpu),
                check=True,
                capture_output=True,
                text=True,
            )
            reader = csv.DictReader(sample.stdout.splitlines())
            current_header = reader.fieldnames
            if current_header is None or tuple(current_header) not in SCHEMAS:
                print(f"unexpected benchmark CSV header: {current_header!r}", file=sys.stderr)
                return 2

            if header is None:
                header = current_header
                metric_columns = SCHEMAS[tuple(header)]
            elif current_header != header:
                print("benchmark CSV header changed across runs", file=sys.stderr)
                return 1

            assert metric_columns is not None
            seen: set[tuple[str, str]] = set()
            for row in reader:
                key = (row["case"], row["input_bytes"])
                if key in seen:
                    print(f"duplicate benchmark case: {key[0]} / {key[1]} B", file=sys.stderr)
                    return 1
                seen.add(key)

                current_stable = (row["output_size_sink"], row["output_fnv1a64"])
                if run_index == 0:
                    order.append(key)
                    stable[key] = current_stable
                    metrics[key] = ([], [])
                elif key not in stable or stable[key] != current_stable:
                    print(
                        f"benchmark output changed across runs for {key[0]} / {key[1]} B",
                        file=sys.stderr,
                    )
                    return 1

                first, second = metrics[key]
                first.append(float(row[metric_columns[0]]))
                second.append(float(row[metric_columns[1]]))
                raw_rows.append([str(run_index + 1), *[row[column] for column in header]])

            if set(order) != seen:
                print("benchmark case set changed across runs", file=sys.stderr)
                return 1
    except (OSError, subprocess.CalledProcessError, ValueError) as exc:
        print(f"repeated benchmark failed: {exc}", file=sys.stderr)
        return 2

    assert header is not None
    assert metric_columns is not None

    if args.raw_output is not None:
        args.raw_output.parent.mkdir(parents=True, exist_ok=True)
        with args.raw_output.open("w", newline="", encoding="utf-8") as handle:
            writer = csv.writer(handle, lineterminator="\n")
            writer.writerow(["run", *header])
            writer.writerows(raw_rows)

    if args.stats_output is not None:
        args.stats_output.parent.mkdir(parents=True, exist_ok=True)
        stats_rows: list[list[str]] = []
        for case_name, input_bytes in order:
            first, second = metrics[(case_name, input_bytes)]
            first_median = rounded_median(first)
            first_min = int(min(first) + 0.5)
            first_max = int(max(first) + 0.5)
            first_mad = compute_mad(first)
            first_rel_mad = compute_relative_mad(first)

            second_median = rounded_median(second)
            second_min = int(min(second) + 0.5)
            second_max = int(max(second) + 0.5)
            second_mad = compute_mad(second)
            second_rel_mad = compute_relative_mad(second)

            quality = classify_quality(
                first_rel_mad,
                warning_pct=args.noise_warning_pct,
                critical_pct=args.noise_critical_pct,
            )

            stats_rows.append([
                case_name,
                input_bytes,
                str(len(first)),
                str(first_median),
                str(first_min),
                str(first_max),
                f"{first_mad:.1f}",
                f"{first_rel_mad:.2f}",
                str(second_median),
                str(second_min),
                str(second_max),
                f"{second_mad:.1f}",
                f"{second_rel_mad:.2f}",
                quality,
            ])
        with args.stats_output.open("w", newline="", encoding="utf-8") as handle:
            writer = csv.writer(handle, lineterminator="\n")
            writer.writerow(STATS_HEADER)
            writer.writerows(stats_rows)

    writer = csv.writer(sys.stdout, lineterminator="\n")
    writer.writerow(header)
    for case_name, input_bytes in order:
        output_size_sink, output_hash = stable[(case_name, input_bytes)]
        first, second = metrics[(case_name, input_bytes)]
        values = {
            "case": case_name,
            "input_bytes": input_bytes,
            metric_columns[0]: str(aggregate_metric(first, mode=args.aggregate)),
            metric_columns[1]: str(aggregate_metric(second, mode=args.aggregate)),
            "output_size_sink": output_size_sink,
            "output_fnv1a64": output_hash,
        }
        writer.writerow([values[column] for column in header])
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
