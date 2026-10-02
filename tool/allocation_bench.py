#!/usr/bin/env python3
"""Run the Linux/glibc allocation probe for all benchmark modes."""

from __future__ import annotations

import argparse
import csv
import os
import re
import subprocess
import sys
from pathlib import Path


MODES = ("devanagari", "latin", "virtual-indic", "expansion", "protected", "mixed-protected")
REPORT_RE = re.compile(
    r"malloc=(\d+),(\d+) calloc=(\d+),(\d+) realloc=(\d+),(\d+) "
    r"free=(\d+) live=(\d+) peak=(\d+) untracked=(\d+)"
)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--probe", type=Path, required=True)
    parser.add_argument("--bytes", type=int, default=1_048_576)
    parser.add_argument("--warmups", type=int, default=0)
    parser.add_argument("--format", choices=("csv", "table"), default="csv")
    return parser.parse_args()


def measure(binary: Path, probe: Path, mode: str, target_bytes: int, warmups: int) -> tuple[int, ...]:
    env = os.environ.copy()
    env["LD_PRELOAD"] = str(probe.resolve())
    sample = subprocess.run(
        [str(binary), mode, str(target_bytes), "1", str(warmups)],
        check=True,
        capture_output=True,
        text=True,
        env=env,
    )
    match = REPORT_RE.search(sample.stderr)
    if match is None:
        raise ValueError(f"unexpected allocator probe output for {mode}: {sample.stderr!r}")

    values = [int(value) for value in match.groups()]
    malloc_calls, malloc_bytes = values[0], values[1]
    calloc_calls, calloc_bytes = values[2], values[3]
    realloc_calls, realloc_bytes = values[4], values[5]
    free_calls, live_bytes, peak_bytes, untracked = values[6], values[7], values[8], values[9]
    if untracked != 0:
        raise ValueError(f"allocator probe tracking overflow for {mode}: untracked={untracked}")

    return (
        malloc_calls,
        calloc_calls,
        realloc_calls,
        free_calls,
        malloc_calls + calloc_calls + realloc_calls,
        malloc_bytes + calloc_bytes + realloc_bytes,
        peak_bytes,
        live_bytes,
    )


def main() -> int:
    args = parse_args()
    if not args.binary.is_file() or not args.probe.is_file():
        print("allocation benchmark binary or probe does not exist", file=sys.stderr)
        return 2

    try:
        rows = [
            (mode, *measure(args.binary, args.probe, mode, args.bytes, args.warmups))
            for mode in MODES
        ]
    except (OSError, subprocess.CalledProcessError, ValueError) as exc:
        print(f"allocation benchmark failed: {exc}", file=sys.stderr)
        return 2

    if args.format == "csv":
        writer = csv.writer(sys.stdout, lineterminator="\n")
        for mode, _malloc, _calloc, _realloc, _free, allocs, allocated, peak, live in rows:
            writer.writerow([mode, allocs, allocated, peak, live])
    else:
        print(
            f"{'mode':<22}  {'malloc':>8}  {'calloc':>8}  {'realloc':>8}  {'free':>8}  "
            f"{'total':>8}  {'alloc_B':>12}  {'peak_B':>12}  {'live_B':>12}"
        )
        for mode, malloc, calloc, realloc, free, total, allocated, peak, live in rows:
            print(
                f"{mode:<22}  {malloc:>8}  {calloc:>8}  {realloc:>8}  {free:>8}  "
                f"{total:>8}  {allocated:>12}  {peak:>12}  {live:>12}"
            )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
