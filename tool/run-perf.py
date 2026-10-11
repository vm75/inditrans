#!/usr/bin/env python3
"""Capture performance snapshots for tagged milestones and/or the current workspace, or run paired benchmarks."""

from __future__ import annotations

import argparse
import os
import signal
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
if str(ROOT / "tool") not in sys.path:
    sys.path.insert(0, str(ROOT / "tool"))

from paired_bench import run_paired_benchmarks


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


def snapshot(root: Path, ref: str, name: str, cpu: str, runs: str, aggregate: str) -> None:
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
            f"BENCH_AGGREGATE={aggregate}",
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


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--paired", action="store_true", help="run paired counterbalanced baseline vs candidate benchmarking")
    parser.add_argument("--baseline", default="perf-00-baseline", help="baseline git ref for paired benchmarking (default: perf-00-baseline)")
    parser.add_argument("--candidate", default="HEAD", help="candidate git ref for paired benchmarking (default: HEAD)")
    parser.add_argument("--runs", type=int, default=None, help="number of repeated benchmark runs (default: 9 locally)")
    parser.add_argument("--cpu", default=None, help="CPU affinity passed to taskset (default: $CPU or 0)")
    parser.add_argument("--aggregate", choices=["median", "mean"], default="median", help="aggregation method (default: median)")
    parser.add_argument("--noise-warning-pct", type=float, default=2.0, help="relative MAD threshold for noisy warning (default: 2.0)")
    parser.add_argument("--noise-critical-pct", type=float, default=5.0, help="relative MAD threshold for critical noise (default: 5.0)")
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
    if args.cpu is not None:
        cpu = args.cpu

    runs = os.environ.get("RUNS", "9")
    if args.runs is not None:
        runs = str(args.runs)
    runs_int = int(runs)
    runs_str = runs

    if args.paired:
        code, _, _ = run_paired_benchmarks(
            repo_root=root,
            baseline_ref=args.baseline,
            candidate_ref=args.candidate,
            runs=runs_int,
            cpu=cpu,
            suite="all",
            output_dir=root / "out" / "perf",
            noise_warning_pct=args.noise_warning_pct,
            noise_critical_pct=args.noise_critical_pct,
            fail_on_hash_mismatch=True,
        )
        return code

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
                snapshot(root, ref, name, cpu, runs_str, args.aggregate)
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
                f"BENCH_RUNS={runs_str}",
                f"BENCH_CPU={cpu}",
                f"PERF_CPU={cpu}",
                f"BENCH_AGGREGATE={args.aggregate}",
            ],
            cwd=root,
        )

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
