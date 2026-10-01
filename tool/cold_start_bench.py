#!/usr/bin/env python3
"""Measure the first transliteration in fresh native processes."""

from __future__ import annotations

import argparse
import csv
import subprocess
import sys
from pathlib import Path


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, required=True, help="cold-start sample executable")
    parser.add_argument("--samples", type=int, default=501, help="number of fresh processes to measure")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    if args.samples < 3 or args.samples % 2 == 0:
        print("cold-start sample count must be an odd number of at least 3", file=sys.stderr)
        return 2
    if not args.binary.is_file():
        print(f"cold-start benchmark executable does not exist: {args.binary}", file=sys.stderr)
        return 2

    elapsed: list[int] = []
    output: tuple[int, int] | None = None
    for _ in range(args.samples):
        sample = subprocess.run([str(args.binary)], check=True, capture_output=True, text=True)
        fields = sample.stdout.strip().split(",")
        if len(fields) != 3:
            print(f"unexpected cold-start sample output: {sample.stdout!r}", file=sys.stderr)
            return 2
        elapsed_ns, output_bytes, output_hash = map(int, fields)
        current_output = (output_bytes, output_hash)
        if output is not None and current_output != output:
            print("cold-start output changed between processes", file=sys.stderr)
            return 1
        output = current_output
        elapsed.append(elapsed_ns)

    elapsed.sort()
    p50_ns = elapsed[len(elapsed) // 2]
    p95_ns = elapsed[(len(elapsed) * 95 + 99) // 100 - 1]
    assert output is not None

    writer = csv.writer(sys.stdout, lineterminator="\n")
    writer.writerow(
        ["case", "input_bytes", "p50_ns", "p95_ns", "samples", "output_bytes", "output_fnv1a64"]
    )
    writer.writerow(
        [
            "cold-devanagari-to-telugu",
            len("श्री॒ गुरुभ्यो नमः । नमस्ते भारतम् । ".encode()),
            p50_ns,
            p95_ns,
            args.samples,
            *output,
        ]
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
