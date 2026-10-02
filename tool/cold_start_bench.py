#!/usr/bin/env python3
"""Measure first-transliteration latency across representative paths in fresh native processes."""

from __future__ import annotations

import argparse
import csv
import shutil
import subprocess
import sys
from pathlib import Path


CASES = (
    "cold-devanagari-to-telugu",
    "cold-iso-to-devanagari",
    "cold-devanagari-to-tamil",
    "cold-indic-to-iso",
)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, required=True, help="cold-start sample executable")
    parser.add_argument("--samples", type=int, default=501, help="fresh processes per case")
    parser.add_argument("--cpu", default="", help="optional Linux CPU id/list passed to taskset")
    return parser.parse_args()


def command(binary: Path, case_name: str, cpu: str) -> list[str]:
    cmd = [str(binary), case_name]
    if not cpu:
        return cmd
    taskset = shutil.which("taskset")
    if taskset is None:
        raise ValueError("--cpu requires taskset")
    return [taskset, "-c", cpu, *cmd]


def percentile(sorted_values: list[int], percentile_value: int) -> int:
    index = (len(sorted_values) * percentile_value + 99) // 100 - 1
    return sorted_values[index]


def main() -> int:
    args = parse_args()
    if args.samples < 3 or args.samples % 2 == 0:
        print("cold-start sample count must be an odd number of at least 3", file=sys.stderr)
        return 2
    if not args.binary.is_file():
        print(f"cold-start benchmark executable does not exist: {args.binary}", file=sys.stderr)
        return 2

    writer = csv.writer(sys.stdout, lineterminator="\n")
    writer.writerow(
        ["case", "input_bytes", "p50_ns", "p95_ns", "samples", "output_bytes", "output_fnv1a64"]
    )

    try:
        for case_name in CASES:
            elapsed: list[int] = []
            expected: tuple[int, int, int] | None = None
            for _ in range(args.samples):
                sample = subprocess.run(
                    command(args.binary, case_name, args.cpu),
                    check=True,
                    capture_output=True,
                    text=True,
                )
                fields = sample.stdout.strip().split(",")
                if len(fields) != 5:
                    print(f"unexpected cold-start sample output: {sample.stdout!r}", file=sys.stderr)
                    return 2
                returned_case = fields[0]
                input_bytes, elapsed_ns, output_bytes, output_hash = map(int, fields[1:])
                if returned_case != case_name:
                    print(f"cold-start case mismatch: requested {case_name!r}, got {returned_case!r}", file=sys.stderr)
                    return 1
                current = (input_bytes, output_bytes, output_hash)
                if expected is not None and current != expected:
                    print(f"cold-start output changed between processes for {case_name}", file=sys.stderr)
                    return 1
                expected = current
                elapsed.append(elapsed_ns)

            elapsed.sort()
            assert expected is not None
            writer.writerow(
                [
                    case_name,
                    expected[0],
                    elapsed[len(elapsed) // 2],
                    percentile(elapsed, 95),
                    args.samples,
                    expected[1],
                    expected[2],
                ]
            )
    except (OSError, subprocess.CalledProcessError, ValueError) as exc:
        print(f"cold-start benchmark failed: {exc}", file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
