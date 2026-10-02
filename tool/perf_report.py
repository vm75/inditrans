#!/usr/bin/env python3
"""Render cumulative performance snapshots as a Markdown progression report."""

from __future__ import annotations

import argparse
import csv
import re
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable


Allocation = tuple[int, int, int, int]


@dataclass(frozen=True)
class Snapshot:
    name: str
    display_name: str
    p50_ns: int
    cold_p50_ns: int | None
    cold_p95_ns: int | None
    cold_alloc: Allocation | None
    warm_alloc: Allocation | None
    wasm_bytes: int | None
    info: str


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Generate a Markdown performance progression report from bench-save snapshots. "
            "Snapshot prefixes are discovered from *-latency.csv files and sorted by name."
        )
    )
    parser.add_argument("--dir", type=Path, default=Path("out/perf"), help="snapshot directory")
    parser.add_argument("--baseline", default="00-baseline", help="baseline snapshot prefix")
    parser.add_argument("--latency-case", default="indic-to-indic", help="short-call case for the summary")
    parser.add_argument("--alloc-case", default="devanagari", help="allocation-probe mode for the summary")
    parser.add_argument(
        "--cold-case",
        default="cold-devanagari-to-telugu",
        help="cold-start case for the cold-latency table",
    )
    parser.add_argument(
        "--step",
        action="append",
        default=[],
        help="snapshot prefix to include after the baseline; repeat to control ordering",
    )
    parser.add_argument("--output", type=Path, help="write Markdown to this file instead of stdout")
    return parser.parse_args()


def read_csv_rows(path: Path) -> list[list[str]]:
    with path.open(newline="", encoding="utf-8") as handle:
        return list(csv.reader(handle))


def read_latency(prefix: Path, case_name: str) -> int:
    path = prefix.with_name(prefix.name + "-latency.csv")
    if not path.is_file():
        raise ValueError(f"missing latency snapshot: {path}")
    rows = read_csv_rows(path)
    if not rows:
        raise ValueError(f"empty latency snapshot: {path}")
    header = rows[0]
    try:
        case_col = header.index("case")
        p50_col = header.index("p50_ns")
    except ValueError as exc:
        raise ValueError(f"unexpected latency CSV header in {path}") from exc
    for row in rows[1:]:
        if len(row) > max(case_col, p50_col) and row[case_col] == case_name:
            return int(row[p50_col])
    raise ValueError(f"latency case {case_name!r} not found in {path}")


def read_cold_latency(prefix: Path, case_name: str) -> tuple[int, int] | None:
    path = prefix.with_name(prefix.name + "-cold.csv")
    if not path.is_file():
        return None
    rows = read_csv_rows(path)
    if len(rows) < 2:
        raise ValueError(f"empty cold-start snapshot: {path}")
    header = rows[0]
    try:
        case_col = header.index("case")
        p50_col = header.index("p50_ns")
        p95_col = header.index("p95_ns")
    except ValueError as exc:
        raise ValueError(f"unexpected cold-start CSV header in {path}") from exc
    for row in rows[1:]:
        if len(row) > max(case_col, p50_col, p95_col) and row[case_col] == case_name:
            return int(row[p50_col]), int(row[p95_col])
    return None


def read_allocations(prefix: Path, mode: str, suffix: str) -> Allocation | None:
    path = prefix.with_name(prefix.name + suffix)
    if not path.is_file() or path.stat().st_size == 0:
        return None
    for row in read_csv_rows(path):
        if len(row) >= 5 and row[0] == mode:
            return tuple(int(value) for value in row[1:5])  # type: ignore[return-value]
    raise ValueError(f"allocation mode {mode!r} not found in {path}")


def read_optional_int(path: Path) -> int | None:
    if not path.is_file():
        return None
    value = path.read_text(encoding="utf-8").strip()
    return int(value) if value else None


def read_info(prefix: Path) -> str:
    path = prefix.with_name(prefix.name + "-info.txt")
    if not path.is_file():
        return ""
    return " ".join(path.read_text(encoding="utf-8").split())


def display_name(prefix_name: str) -> str:
    name = re.sub(r"^\d+[._-]*", "", prefix_name)
    return (name or prefix_name).replace("_", " ").replace("-", " ")


def load_snapshot(directory: Path, name: str, latency_case: str, alloc_case: str, cold_case: str) -> Snapshot:
    prefix = directory / name
    cold_latency = read_cold_latency(prefix, cold_case)
    return Snapshot(
        name=name,
        display_name=display_name(name),
        p50_ns=read_latency(prefix, latency_case),
        cold_p50_ns=cold_latency[0] if cold_latency else None,
        cold_p95_ns=cold_latency[1] if cold_latency else None,
        cold_alloc=read_allocations(prefix, alloc_case, "-allocs.csv"),
        warm_alloc=read_allocations(prefix, alloc_case, "-allocs-warm.csv"),
        wasm_bytes=read_optional_int(prefix.with_name(prefix.name + "-wasm-size.txt")),
        info=read_info(prefix),
    )


def discover_steps(directory: Path, baseline: str) -> list[str]:
    suffix = "-latency.csv"
    names = sorted(
        path.name[: -len(suffix)]
        for path in directory.glob(f"*{suffix}")
        if path.name.endswith(suffix)
    )
    return [name for name in names if name != baseline]


def pct_delta(value: int | None, baseline: int | None) -> str:
    if value is None or baseline is None:
        return "—"
    if baseline == 0:
        return "0.0%" if value == 0 else "n/a"
    return f"{(value - baseline) / baseline * 100:+.1f}%"


def count_delta(value: int | None, baseline: int | None) -> str:
    if value is None or baseline is None:
        return "—"
    return f"{value - baseline:+d}"


def alloc_value(allocation: Allocation | None, index: int) -> int | None:
    return allocation[index] if allocation is not None else None


def first_metric(
    snapshots: list[Snapshot], getter
) -> tuple[Snapshot | None, int | None]:
    for snapshot in snapshots:
        value = getter(snapshot)
        if value is not None:
            return snapshot, value
    return None, None


def format_duration(ns: int) -> str:
    if ns < 1_000:
        return f"{ns} ns"
    if ns < 1_000_000:
        return f"{ns / 1_000:.3f} µs"
    return f"{ns / 1_000_000:.3f} ms"


def format_bytes(value: int | None) -> str:
    if value is None:
        return "—"
    units = ("B", "KiB", "MiB", "GiB")
    number = float(value)
    unit = units[0]
    for unit in units:
        if abs(number) < 1024 or unit == units[-1]:
            break
        number /= 1024
    if unit == "B":
        return f"{int(number)} {unit}"
    return f"{number:.2f} {unit}"


def markdown_table(headers: list[str], rows: Iterable[list[str]]) -> list[str]:
    lines = ["| " + " | ".join(headers) + " |"]
    lines.append("| " + " | ".join(["---"] + ["---:" for _ in headers[1:]]) + " |")
    lines.extend("| " + " | ".join(row) + " |" for row in rows)
    return lines


def render_report(
    snapshots: list[Snapshot],
    baseline: Snapshot,
    latency_case: str,
    alloc_case: str,
    cold_case: str,
) -> str:
    lines = [
        "# Performance progression",
        "",
        f"Baseline: `{baseline.name}`" + (f" ({baseline.info})" if baseline.info else ""),
        "",
        (
            f"Summary uses short-call p50 case `{latency_case}` and allocation mode `{alloc_case}`. "
            "Cold allocations use no warmup and therefore include lazy initialization; warm allocations "
            "use one unmeasured warmup. Peak heap is allocator-probe peak requested live bytes, not process RSS. "
            "All change rows are relative to the original baseline."
        ),
        "",
    ]

    warm_alloc_ref_snapshot, warm_alloc_ref = first_metric(
        snapshots, lambda snapshot: alloc_value(snapshot.warm_alloc, 0)
    )
    warm_peak_ref_snapshot, warm_peak_ref = first_metric(
        snapshots, lambda snapshot: alloc_value(snapshot.warm_alloc, 2)
    )

    if warm_alloc_ref_snapshot is not None and warm_alloc_ref_snapshot is not baseline:
        lines.extend(
            [
                (
                    f"Warm-allocation deltas use `{warm_alloc_ref_snapshot.name}` as their reference "
                    "because earlier snapshots do not contain warm-allocation data."
                ),
                "",
            ]
        )

    summary_rows: list[list[str]] = []
    for index, snapshot in enumerate(snapshots):
        cold_allocs = alloc_value(snapshot.cold_alloc, 0)
        warm_allocs = alloc_value(snapshot.warm_alloc, 0)
        cold_peak = alloc_value(snapshot.cold_alloc, 2)
        warm_peak = alloc_value(snapshot.warm_alloc, 2)
        if index == 0:
            summary_rows.append(
                [
                    "baseline",
                    format_duration(snapshot.p50_ns),
                    str(cold_allocs) if cold_allocs is not None else "—",
                    str(warm_allocs) if warm_allocs is not None else "—",
                    format_bytes(cold_peak),
                    format_bytes(warm_peak),
                    format_bytes(snapshot.wasm_bytes),
                ]
            )
        else:
            summary_rows.append(
                [
                    snapshot.display_name,
                    pct_delta(snapshot.p50_ns, baseline.p50_ns),
                    count_delta(cold_allocs, alloc_value(baseline.cold_alloc, 0)),
                    count_delta(warm_allocs, warm_alloc_ref),
                    pct_delta(cold_peak, alloc_value(baseline.cold_alloc, 2)),
                    pct_delta(warm_peak, warm_peak_ref),
                    pct_delta(snapshot.wasm_bytes, baseline.wasm_bytes),
                ]
            )
    lines.extend(
        markdown_table(
            ["Step", "p50 latency", "cold allocs", "warm allocs", "cold peak", "warm peak", "Wasm size"],
            summary_rows,
        )
    )

    cold_ref_snapshot, cold_ref_p50 = first_metric(snapshots, lambda snapshot: snapshot.cold_p50_ns)
    if cold_ref_snapshot is not None:
        lines.extend(["", f"## Cold-start latency — `{cold_case}`", ""])
        if cold_ref_snapshot is not baseline:
            lines.extend(
                [
                    (
                        f"Reference: `{cold_ref_snapshot.name}` (the earliest snapshot containing this cold case)."
                    ),
                    "",
                ]
            )
        cold_rows = []
        for index, snapshot in enumerate(snapshots):
            reference_label = "—"
            if snapshot.cold_p50_ns is not None:
                reference_label = (
                    "baseline"
                    if cold_ref_snapshot is baseline and snapshot is baseline
                    else "reference"
                    if snapshot is cold_ref_snapshot
                    else pct_delta(snapshot.cold_p50_ns, cold_ref_p50)
                )
            cold_rows.append(
                [
                    snapshot.display_name if index else "baseline",
                    format_duration(snapshot.cold_p50_ns) if snapshot.cold_p50_ns is not None else "—",
                    format_duration(snapshot.cold_p95_ns) if snapshot.cold_p95_ns is not None else "—",
                    reference_label,
                ]
            )
        lines.extend(markdown_table(["Step", "p50", "p95", "p50 vs reference"], cold_rows))

    lines.extend(["", "## Absolute allocation values", ""])
    allocation_rows = []
    for index, snapshot in enumerate(snapshots):
        allocation_rows.append(
            [
                snapshot.display_name if index else "baseline",
                str(alloc_value(snapshot.cold_alloc, 0)) if snapshot.cold_alloc else "—",
                str(alloc_value(snapshot.warm_alloc, 0)) if snapshot.warm_alloc else "—",
                format_bytes(alloc_value(snapshot.cold_alloc, 1)),
                format_bytes(alloc_value(snapshot.warm_alloc, 1)),
                format_bytes(alloc_value(snapshot.cold_alloc, 2)),
                format_bytes(alloc_value(snapshot.warm_alloc, 2)),
                format_bytes(alloc_value(snapshot.cold_alloc, 3)),
                format_bytes(alloc_value(snapshot.warm_alloc, 3)),
            ]
        )
    lines.extend(
        markdown_table(
            [
                "Step",
                "cold allocs",
                "warm allocs",
                "cold allocated",
                "warm allocated",
                "cold peak",
                "warm peak",
                "cold live",
                "warm live",
            ],
            allocation_rows,
        )
    )

    lines.extend(["", "## Snapshot metadata", ""])
    lines.extend(
        markdown_table(
            ["Step", "p50", "Wasm", "snapshot"],
            [
                [
                    snapshot.display_name if index else "baseline",
                    format_duration(snapshot.p50_ns),
                    format_bytes(snapshot.wasm_bytes),
                    snapshot.info or "—",
                ]
                for index, snapshot in enumerate(snapshots)
            ],
        )
    )
    return "\n".join(lines) + "\n"


def main() -> int:
    args = parse_args()
    if not args.dir.is_dir():
        print(f"performance snapshot directory does not exist: {args.dir}", file=sys.stderr)
        return 2

    step_names = args.step or discover_steps(args.dir, args.baseline)
    try:
        baseline = load_snapshot(args.dir, args.baseline, args.latency_case, args.alloc_case, args.cold_case)
        snapshots = [baseline]
        snapshots.extend(
            load_snapshot(args.dir, name, args.latency_case, args.alloc_case, args.cold_case)
            for name in step_names
        )
        report = render_report(snapshots, baseline, args.latency_case, args.alloc_case, args.cold_case)
    except (OSError, ValueError) as exc:
        print(f"perf-report: {exc}", file=sys.stderr)
        return 2

    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(report, encoding="utf-8")
        print(f"Performance report written to {args.output}")
    else:
        print(report, end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
