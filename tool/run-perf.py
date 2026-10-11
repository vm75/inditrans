#!/usr/bin/env python3
"""Capture performance snapshots for tagged milestones and/or the current workspace."""

from __future__ import annotations

import argparse
import csv
import os
import signal
import subprocess
import sys
from pathlib import Path

from repeat_short_bench import SCHEMAS, rounded_median


def run(command: list[str], *, cwd: Path, check: bool = True, capture: bool = False) -> str:
    result = subprocess.run(
        command,
        cwd=cwd,
        check=check,
        text=True,
        stdout=subprocess.PIPE if capture else None,
    )
    return result.stdout.strip() if capture and result.stdout else ""


def git(root: Path, *args: str, check: bool = True, capture: bool = False) -> str:
    return run(["git", *args], cwd=root, check=check, capture=capture)


def snapshot(root: Path, ref: str, name: str, cpu: str, runs: str) -> None:
    print(f"==> Running snapshot for {name} ({ref})...", flush=True)
    git(root, "restore", ".")
    git(root, "checkout", "--detach", ref)
    run(
        [
            "make",
            "perf-snapshot",
            f"PERF_NAME={name}",
            f"BENCH_RUNS={runs}",
            f"BENCH_CPU={cpu}",
            f"PERF_CPU={cpu}",
        ],
        cwd=root,
    )


def restore_workspace(
    root: Path, current_commit: str, current_branch: str, stash_commit: str, checkout_started: bool
) -> None:
    if checkout_started:
        git(root, "restore", ".")
        if current_branch:
            git(root, "switch", current_branch)
        else:
            git(root, "checkout", "--detach", current_commit)
    if stash_commit:
        git(root, "stash", "apply", "--index", stash_commit)
        stash_list = git(root, "stash", "list", "--format=%H %gd", capture=True)
        for line in stash_list.splitlines():
            commit, ref = line.split(maxsplit=1)
            if commit == stash_commit:
                git(root, "stash", "drop", ref)
                break


def normalize_snapshot(prefix: Path) -> None:
    """Recompute old-tag aggregates from raw runs using the current median policy."""
    for kind in ("throughput", "latency"):
        output = prefix.with_name(f"{prefix.name}-{kind}.csv")
        if not output.is_file():
            continue
        raw = prefix.with_name(f"{prefix.name}-{kind}-runs.csv")
        if not raw.is_file():
            raise ValueError(f"Cannot normalize {output}: missing {raw}; rerun the snapshot")

        with raw.open(newline="", encoding="utf-8") as handle:
            reader = csv.DictReader(handle)
            fields = reader.fieldnames
            if not fields or fields[0] != "run" or tuple(fields[1:]) not in SCHEMAS:
                raise ValueError(f"Unexpected raw benchmark CSV header: {raw}")
            metrics = SCHEMAS[tuple(fields[1:])]
            groups: dict[tuple[str, str], list[dict[str, str]]] = {}
            for row in reader:
                key = (row["case"], row["input_bytes"])
                if key in groups:
                    first = groups[key][0]
                    for field in ("output_size_sink", "output_fnv1a64"):
                        if row[field] != first[field]:
                            raise ValueError(f"Output mismatch across runs for {key} in {raw}")
                groups.setdefault(key, []).append(row)
        if not groups:
            raise ValueError(f"No benchmark results in {raw}")

        summaries = []
        for rows in groups.values():
            record = {field: rows[0][field] for field in fields[1:]}
            for metric in metrics:
                record[metric] = str(rounded_median([float(row[metric]) for row in rows]))
            summaries.append(record)

        temporary = output.with_name(output.name + ".tmp")
        with temporary.open("w", newline="", encoding="utf-8") as handle:
            writer = csv.DictWriter(handle, fieldnames=fields[1:], lineterminator="\n")
            writer.writeheader()
            writer.writerows(summaries)
        temporary.replace(output)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--run-tags", action="store_true", help="snapshot perf-* tags (all tags when none are listed)")
    parser.add_argument("--current", action="store_true", help="snapshot current workspace as 99-current (default without options)")
    parser.add_argument("tags", nargs="*", help="specific perf-* tags to snapshot; implies --run-tags")
    args = parser.parse_args()

    def stop_on_signal(signum: int, _frame: object) -> None:
        raise SystemExit(128 + signum)

    for signum in (signal.SIGINT, signal.SIGTERM):
        signal.signal(signum, stop_on_signal)

    root = Path(git(Path.cwd(), "rev-parse", "--show-toplevel", capture=True))

    cpu = os.environ.get("CPU", "0")
    runs = os.environ.get("RUNS", "5")

    run_tags = args.run_tags or bool(args.tags)
    run_current = args.current or not run_tags

    if run_tags:
        current_commit = git(root, "rev-parse", "HEAD", capture=True)
        current_branch = git(root, "symbolic-ref", "--quiet", "--short", "HEAD", check=False, capture=True)
        dirty = git(root, "status", "--porcelain", "--untracked-files=all", capture=True)
        stash_commit = ""
        checkout_started = False
        if dirty:
            git(root, "stash", "push", "--include-untracked", "-m", "Before performance benchmarks")
            stash_commit = git(root, "rev-parse", "refs/stash", capture=True)
        checkout_started = True
        try:
            available_tags = git(root, "tag", "--list", "perf-*", capture=True).splitlines()
            tags = args.tags if args.tags else available_tags
            for ref in sorted(tags):
                if ref not in available_tags:
                    raise ValueError(f"Unknown performance tag: {ref}")
                name = ref.removeprefix("perf-")
                if not args.tags and (root / "out" / "perf" / f"{name}-wasm-size.txt").is_file():
                    print(f"Skipping {name}\n", flush=True)
                    continue
                snapshot(root, ref, name, cpu, runs)
        finally:
            restore_workspace(root, current_commit, current_branch, stash_commit, checkout_started)

    if run_current:
        current_label = git(root, "rev-parse", "--short", "HEAD", capture=True)
        print(f"==> Running snapshot for 99-current ({current_label})...", flush=True)
        run(
            [
                "make",
                "perf-snapshot",
                "PERF_NAME=99-current",
                f"BENCH_RUNS={runs}",
                f"BENCH_CPU={cpu}",
                f"PERF_CPU={cpu}",
            ],
            cwd=root,
        )

    # Historical tags may contain the old mean-based repeat helper. Normalize every
    # snapshot from its raw runs so the progression report compares like with like.
    perf_dir = root / "out" / "perf"
    for throughput_csv in sorted(perf_dir.glob("*-throughput.csv")):
        name = throughput_csv.name.removesuffix("-throughput.csv")
        normalize_snapshot(perf_dir / name)

    report_path = root / "out" / "report.md"
    report_path.parent.mkdir(parents=True, exist_ok=True)
    with report_path.open("w", encoding="utf-8") as report:
        subprocess.run(
            [
                "make",
                "perf-report",
                "PERF_BASELINE=00-baseline",
                f"PERF_DIR={root / 'out' / 'perf'}",
            ],
            cwd=root,
            stdout=report,
            check=True,
            text=True,
        )
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except KeyboardInterrupt:
        raise SystemExit(130)
