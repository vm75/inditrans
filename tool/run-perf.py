#!/usr/bin/env python3
"""Capture performance snapshots for tagged milestones and/or the current workspace."""

from __future__ import annotations

import argparse
import os
import signal
import subprocess
import sys
from pathlib import Path


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
        ["make", "perf-snapshot", f"PERF_NAME={name}", f"BENCH_RUNS={runs}", f"BENCH_CPU={cpu}", f"PERF_CPU={cpu}"],
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
    parser.add_argument("--run-tags", action="store_true", help="snapshot all perf-* tags, stashing and restoring workspace changes")
    parser.add_argument("--current", action="store_true", help="snapshot current workspace as 99-current (default without options)")
    args = parser.parse_args()
    def stop_on_signal(signum: int, _frame: object) -> None:
        raise SystemExit(128 + signum)

    for signum in (signal.SIGINT, signal.SIGTERM):
        signal.signal(signum, stop_on_signal)
    run_current = args.current or not args.run_tags

    root = Path(git(Path.cwd(), "rev-parse", "--show-toplevel", capture=True))
    cpu = os.environ.get("CPU", "0")
    runs = os.environ.get("RUNS", "5")

    if args.run_tags:
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
            tags = git(root, "tag", "--list", "perf-*", capture=True).splitlines()
            for ref in sorted(tags):
                name = ref.removeprefix("perf-")
                if (root / "out" / "perf" / f"{name}-wasm-size.txt").is_file():
                    print(f"Skipping {name}\n", flush=True)
                    continue
                snapshot(root, ref, name, cpu, runs)
        finally:
            restore_workspace(root, current_commit, current_branch, stash_commit, checkout_started)

    if run_current:
        current_label = git(root, "rev-parse", "--short", "HEAD", capture=True)
        print(f"==> Running snapshot for 99-current ({current_label})...", flush=True)
        run(
            ["make", "perf-snapshot", "PERF_NAME=99-current", f"BENCH_RUNS={runs}", f"BENCH_CPU={cpu}", f"PERF_CPU={cpu}"],
            cwd=root,
        )

    report_path = root / "out" / "report.md"
    report_path.parent.mkdir(parents=True, exist_ok=True)
    with report_path.open("w", encoding="utf-8") as report:
        subprocess.run(["make", "perf-report", "PERF_BASELINE=00-baseline"], cwd=root, stdout=report, check=True, text=True)
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except KeyboardInterrupt:
        raise SystemExit(130)
