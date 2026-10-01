#!/usr/bin/env python3
"""Run the short-call benchmark repeatedly and emit median run-level p50/p95 values."""

from __future__ import annotations

import argparse
import csv
import shutil
import subprocess
import sys
from pathlib import Path


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, required=True, help="short-call benchmark executable")
    parser.add_argument("--runs", type=int, default=5, help="odd number of complete benchmark runs")
    parser.add_argument("--cpu", default="", help="optional Linux CPU id/list passed to taskset")
    return parser.parse_args()


def command(binary: Path, cpu: str) -> list[str]:
    cmd = [str(binary)]
    if not cpu:
        return cmd
    taskset = shutil.which("taskset")
    if taskset is None:
        raise ValueError("--cpu requires taskset")
    return [taskset, "-c", cpu, *cmd]


def median(values: list[int]) -> int:
    values.sort()
    return values[len(values) // 2]


def main() -> int:
    args = parse_args()
    if args.runs < 3 or args.runs % 2 == 0:
        print("repeat count must be an odd number of at least 3", file=sys.stderr)
        return 2
    if not args.binary.is_file():
        print(f"short-call benchmark executable does not exist: {args.binary}", file=sys.stderr)
        return 2

    expected_header = ["case", "input_bytes", "p50_ns", "p95_ns", "output_size_sink", "output_fnv1a64"]
    order: list[str] = []
    stable: dict[str, tuple[str, str, str]] = {}
    p50: dict[str, list[int]] = {}
    p95: dict[str, list[int]] = {}

    try:
        for run_index in range(args.runs):
            sample = subprocess.run(command(args.binary, args.cpu), check=True, capture_output=True, text=True)
            reader = csv.DictReader(sample.stdout.splitlines())
            if reader.fieldnames != expected_header:
                print(f"unexpected short-call CSV header: {reader.fieldnames!r}", file=sys.stderr)
                return 2

            seen: set[str] = set()
            for row in reader:
                case_name = row["case"]
                if case_name in seen:
                    print(f"duplicate short-call case: {case_name}", file=sys.stderr)
                    return 1
                seen.add(case_name)
                current = (row["input_bytes"], row["output_size_sink"], row["output_fnv1a64"])
                if run_index == 0:
                    order.append(case_name)
                    stable[case_name] = current
                    p50[case_name] = []
                    p95[case_name] = []
                elif case_name not in stable or stable[case_name] != current:
                    print(f"short-call output changed across runs for {case_name}", file=sys.stderr)
                    return 1
                p50[case_name].append(int(row["p50_ns"]))
                p95[case_name].append(int(row["p95_ns"]))

            if set(order) != seen:
                print("short-call case set changed across runs", file=sys.stderr)
                return 1
    except (OSError, subprocess.CalledProcessError, ValueError) as exc:
        print(f"repeated short-call benchmark failed: {exc}", file=sys.stderr)
        return 2

    writer = csv.writer(sys.stdout, lineterminator="\n")
    writer.writerow(expected_header)
    for case_name in order:
        input_bytes, output_size_sink, output_hash = stable[case_name]
        writer.writerow(
            [
                case_name,
                input_bytes,
                median(p50[case_name]),
                median(p95[case_name]),
                output_size_sink,
                output_hash,
            ]
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
