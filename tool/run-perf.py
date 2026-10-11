#!/usr/bin/env python3
"""Capture performance snapshots for tagged milestones and/or the current workspace."""

from __future__ import annotations

import argparse
import csv
import os
import re
import signal
import subprocess
import sys
from pathlib import Path

from perf_metadata import compute_harness_fingerprint
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


def get_registered_worktrees(root: Path) -> dict[Path, dict[str, str]]:
    output = git(root, "worktree", "list", "--porcelain", capture=True)
    worktrees: dict[Path, dict[str, str]] = {}
    for block in output.strip().split("\n\n"):
        if not block.strip():
            continue
        entry: dict[str, str] = {}
        for line in block.splitlines():
            parts = line.split(maxsplit=1)
            if len(parts) == 2:
                entry[parts[0]] = parts[1]
            elif len(parts) == 1:
                entry[parts[0]] = ""
        if "worktree" in entry:
            worktrees[Path(entry["worktree"]).resolve()] = entry
    return worktrees


def ensure_worktree(root: Path, ref: str) -> Path:
    """Ensure a persistent, sparse Git worktree exists for the given tag/ref."""
    worktree_path = root / "out" / "perf" / "worktrees" / ref
    try:
        expected_commit = git(root, "rev-parse", f"{ref}^{{commit}}", capture=True)
    except subprocess.CalledProcessError as err:
        raise ValueError(f"Unknown performance tag or ref: {ref}") from err

    registered = get_registered_worktrees(root)
    resolved_target = worktree_path.resolve()

    if worktree_path.exists():
        if resolved_target not in registered:
            raise ValueError(
                f"Directory '{worktree_path}' exists but is not a registered Git worktree. "
                "Please inspect or remove it manually."
            )
        entry = registered[resolved_target]
        actual_head = entry.get("HEAD", "")
        if actual_head != expected_commit:
            raise ValueError(
                f"Worktree at '{worktree_path}' has HEAD {actual_head}, expected {expected_commit} ({ref}). "
                f"Remove it explicitly with 'git worktree remove {worktree_path}' if obsolete."
            )
        if not (worktree_path / "native" / "src" / "inditrans.cpp").is_file() or not (
            worktree_path / "native" / "src" / "script_data.h"
        ).is_file():
            raise ValueError(
                f"Worktree at '{worktree_path}' is missing engine sources in native/src. "
                f"Run 'git -C {worktree_path} checkout' or inspect the worktree."
            )
        return worktree_path

    if resolved_target in registered:
        raise ValueError(
            f"Worktree metadata exists for '{worktree_path}', but directory is missing on disk. "
            "Clean up stale worktree metadata with 'git worktree prune'."
        )

    worktree_path.parent.mkdir(parents=True, exist_ok=True)
    git(root, "worktree", "add", "--detach", "--no-checkout", str(worktree_path), ref)
    git(worktree_path, "sparse-checkout", "set", "--cone", "native/src")
    git(worktree_path, "checkout")
    if not (worktree_path / "native" / "src" / "inditrans.cpp").is_file() or not (
        worktree_path / "native" / "src" / "script_data.h"
    ).is_file():
        raise ValueError(
            f"Failed to check out native/src engine sources into '{worktree_path}'. "
            "Verify that the requested tag includes native/src/script_data.h."
        )
    return worktree_path


def get_expected_benchmark_cases(root: Path) -> set[str]:
    bench_cpp = root / "native" / "bench" / "benchmark.cpp"
    if not bench_cpp.is_file():
        return set()
    return set(
        re.findall(
            r'Case\s*\{\s*"[^"]*",\s*"[^"]*",\s*"([^"]+)"',
            bench_cpp.read_text(encoding="utf-8"),
        )
    )


def read_info_dict(info_path: Path) -> dict[str, str]:
    if not info_path.is_file():
        return {}
    info = {}
    for line in info_path.read_text(encoding="utf-8").splitlines():
        line = line.strip()
        if "=" in line:
            key, val = line.split("=", 1)
            info[key.strip()] = val.strip()
    return info


def is_snapshot_valid(
    prefix: Path,
    expected_engine_commit: str,
    harness_fingerprint: str,
    runs: str,
    cpu: str,
    expected_cases: set[str],
) -> bool:
    required_files = [
        prefix.with_name(f"{prefix.name}-throughput.csv"),
        prefix.with_name(f"{prefix.name}-throughput-runs.csv"),
        prefix.with_name(f"{prefix.name}-latency.csv"),
        prefix.with_name(f"{prefix.name}-latency-runs.csv"),
        prefix.with_name(f"{prefix.name}-cold.csv"),
        prefix.with_name(f"{prefix.name}-wasm-size.txt"),
        prefix.with_name(f"{prefix.name}-info.txt"),
    ]
    if not all(path.is_file() and path.stat().st_size > 0 for path in required_files):
        return False

    info = read_info_dict(prefix.with_name(f"{prefix.name}-info.txt"))
    engine_commit = info.get("commit_full") or info.get("engine_commit_full")
    if engine_commit != expected_engine_commit:
        return False
    if info.get("harness_fingerprint") != harness_fingerprint:
        return False
    if info.get("bench_runs") != runs:
        return False
    if info.get("bench_cpu") != cpu:
        return False

    # Verify benchmark cases
    for kind in ("throughput", "latency"):
        csv_path = prefix.with_name(f"{prefix.name}-{kind}.csv")
        try:
            with csv_path.open(newline="", encoding="utf-8") as handle:
                reader = csv.DictReader(handle)
                cases = {row["case"] for row in reader if "case" in row}
                if not expected_cases.issubset(cases):
                    return False
        except (OSError, KeyError, csv.Error):
            return False
    return True


def validate_snapshot_compatibility(prefix: Path, expected_cases: set[str], name: str) -> None:
    for kind in ("throughput", "latency"):
        csv_path = prefix.with_name(f"{prefix.name}-{kind}.csv")
        if not csv_path.is_file():
            continue
        with csv_path.open(newline="", encoding="utf-8") as handle:
            reader = csv.DictReader(handle)
            cases = {row["case"] for row in reader if "case" in row}
            missing = expected_cases - cases
            if missing:
                raise ValueError(
                    f"Snapshot '{name}' ({csv_path.name}) is missing benchmark cases: {sorted(missing)}. "
                    "Historical performance snapshots must use compatible benchmark matrices."
                )


def snapshot(
    root: Path,
    ref: str,
    name: str,
    cpu: str,
    runs: str,
    *,
    is_historical: bool = False,
    worktree_path: Path | None = None,
) -> None:
    perf_dir = root / "out" / "perf"
    perf_dir.mkdir(parents=True, exist_ok=True)

    if is_historical:
        if worktree_path is None:
            raise ValueError(f"Historical snapshot for {ref} requires worktree_path")
        engine_src = worktree_path / "native" / "src"
        build_dir = root / "out" / "perf" / "build" / ref
        use_checked_in = "1"
        metadata_flags = f'--engine-dir "{worktree_path}" --engine-ref "{ref}"'
    else:
        engine_src = root / "native" / "src"
        build_dir = root / "out" / "perf" / "build" / "current"
        use_checked_in = "0"
        metadata_flags = f'--engine-dir "{root}" --engine-ref "current"'

    build_dir.mkdir(parents=True, exist_ok=True)
    print(f"==> Running snapshot for {name} ({ref})...", flush=True)

    cmd = [
        "make",
        "-f",
        str(root / "Makefile"),
        "perf-snapshot",
        f"PERF_NAME={name}",
        f"PERF_DIR={perf_dir}",
        f"NATIVE_SRC={engine_src}",
        f"NATIVE_BENCH={build_dir / 'inditrans_bench'}",
        f"NATIVE_SHORT_BENCH={build_dir / 'inditrans_short_bench'}",
        f"NATIVE_COLD_BENCH={build_dir / 'inditrans_cold_bench'}",
        f"NATIVE_ALLOC_BENCH={build_dir / 'inditrans_alloc_bench'}",
        f"LINUX_ALLOC_PROBE={build_dir / 'libinditrans_alloc_probe.so'}",
        f"WASM_OUT={build_dir / 'inditrans.wasm'}",
        f"BENCH_RUNS={runs}",
        f"BENCH_CPU={cpu}",
        f"PERF_CPU={cpu}",
        f"USE_CHECKED_IN_SCRIPT_DATA={use_checked_in}",
        f"PERF_METADATA_FLAGS={metadata_flags}",
    ]
    run(cmd, cwd=root)


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

    harness_fingerprint = compute_harness_fingerprint(root)
    expected_cases = get_expected_benchmark_cases(root)

    if run_tags:
        available_tags = git(root, "tag", "--list", "perf-*", capture=True).splitlines()
        tags = args.tags if args.tags else available_tags
        for ref in sorted(tags):
            if ref not in available_tags:
                raise ValueError(f"Unknown performance tag: {ref}")
            name = ref.removeprefix("perf-")
            tag_commit = git(root, "rev-parse", f"{ref}^{{commit}}", capture=True)
            prefix = root / "out" / "perf" / name

            # Only auto-skip when tags were NOT explicitly specified on command line
            if not args.tags and is_snapshot_valid(
                prefix, tag_commit, harness_fingerprint, runs, cpu, expected_cases
            ):
                print(f"Skipping {name}\n", flush=True)
                continue

            worktree_path = ensure_worktree(root, ref)
            snapshot(
                root,
                ref,
                name,
                cpu,
                runs,
                is_historical=True,
                worktree_path=worktree_path,
            )

    if run_current:
        current_label = git(root, "rev-parse", "--short", "HEAD", capture=True)
        snapshot(root, current_label, "99-current", cpu, runs, is_historical=False)

    perf_dir = root / "out" / "perf"
    for throughput_csv in sorted(perf_dir.glob("*-throughput.csv")):
        name = throughput_csv.name.removesuffix("-throughput.csv")
        normalize_snapshot(perf_dir / name)

    # Validate that participating snapshots contain compatible benchmark cases before generating report
    baseline_prefix = perf_dir / "00-baseline"
    if baseline_prefix.with_name("00-baseline-throughput.csv").is_file():
        validate_snapshot_compatibility(baseline_prefix, expected_cases, "00-baseline")
    for latency_csv in sorted(perf_dir.glob("*-latency.csv")):
        name = latency_csv.name.removesuffix("-latency.csv")
        validate_snapshot_compatibility(perf_dir / name, expected_cases, name)

    report_path = root / "out" / "report.md"
    report_path.parent.mkdir(parents=True, exist_ok=True)
    with report_path.open("w", encoding="utf-8") as report:
        subprocess.run(
            [
                "make",
                "-f",
                str(root / "Makefile"),
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
