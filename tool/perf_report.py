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


@dataclass(frozen=True)
class Snapshot:
    name: str
    display_name: str
    p50_ns: int
    allocations: int | None
    allocated_bytes: int | None
    peak_heap_bytes: int | None
    live_heap_bytes: int | None
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
    parser.add_argument(
        "--latency-case",
        default="indic-to-indic",
        help="short-call benchmark case used by the summary table",
    )
    parser.add_argument(
        "--alloc-case",
        default="devanagari",
        help="allocation-probe mode used by the summary table",
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


def read_allocations(prefix: Path, mode: str) -> tuple[int, int, int, int] | None:
    path = prefix.with_name(prefix.name + "-allocs.csv")
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


def load_snapshot(directory: Path, name: str, latency_case: str, alloc_case: str) -> Snapshot:
    prefix = directory / name
    alloc = read_allocations(prefix, alloc_case)
    return Snapshot(
        name=name,
        display_name=display_name(name),
        p50_ns=read_latency(prefix, latency_case),
        allocations=alloc[0] if alloc else None,
        allocated_bytes=alloc[1] if alloc else None,
        peak_heap_bytes=alloc[2] if alloc else None,
        live_heap_bytes=alloc[3] if alloc else None,
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
    snapshots: list[Snapshot], baseline: Snapshot, latency_case: str, alloc_case: str
) -> str:
    lines = [
        "# Performance progression",
        "",
        f"Baseline: `{baseline.name}`" + (f" ({baseline.info})" if baseline.info else ""),
        "",
        (
            f"Summary uses short-call p50 case `{latency_case}` and allocation mode `{alloc_case}`. "
            "All change rows are relative to the original baseline. Peak heap is allocator-probe "
            "peak requested live bytes, not process RSS."
        ),
        "",
    ]

    summary_rows: list[list[str]] = []
    for index, snapshot in enumerate(snapshots):
        if index == 0:
            summary_rows.append(
                [
                    "baseline",
                    format_duration(snapshot.p50_ns),
                    str(snapshot.allocations) if snapshot.allocations is not None else "—",
                    format_bytes(snapshot.peak_heap_bytes),
                    format_bytes(snapshot.wasm_bytes),
                ]
            )
        else:
            summary_rows.append(
                [
                    snapshot.display_name,
                    pct_delta(snapshot.p50_ns, baseline.p50_ns),
                    count_delta(snapshot.allocations, baseline.allocations),
                    pct_delta(snapshot.peak_heap_bytes, baseline.peak_heap_bytes),
                    pct_delta(snapshot.wasm_bytes, baseline.wasm_bytes),
                ]
            )
    lines.extend(markdown_table(["Step", "p50 latency", "allocs", "peak heap", "Wasm size"], summary_rows))

    lines.extend(["", "## Absolute values", ""])
    absolute_rows = [
        [
            snapshot.display_name if index else "baseline",
            format_duration(snapshot.p50_ns),
            str(snapshot.allocations) if snapshot.allocations is not None else "—",
            format_bytes(snapshot.allocated_bytes),
            format_bytes(snapshot.peak_heap_bytes),
            format_bytes(snapshot.live_heap_bytes),
            format_bytes(snapshot.wasm_bytes),
            snapshot.info or "—",
        ]
        for index, snapshot in enumerate(snapshots)
    ]
    lines.extend(
        markdown_table(
            ["Step", "p50", "allocs", "allocated", "peak heap", "live heap", "Wasm", "snapshot"],
            absolute_rows,
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
        baseline = load_snapshot(args.dir, args.baseline, args.latency_case, args.alloc_case)
        snapshots = [baseline]
        snapshots.extend(
            load_snapshot(args.dir, name, args.latency_case, args.alloc_case) for name in step_names
        )
        report = render_report(snapshots, baseline, args.latency_case, args.alloc_case)
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
