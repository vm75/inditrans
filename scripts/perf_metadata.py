#!/usr/bin/env python3
"""Emit reproducibility metadata for a local performance snapshot."""

from __future__ import annotations

import argparse
import hashlib
import os
import platform
import re
import shutil
import subprocess
from datetime import datetime, timezone
from pathlib import Path


def command_line(command: list[str]) -> str:
    try:
        result = subprocess.run(command, check=True, capture_output=True, text=True)
    except (OSError, subprocess.CalledProcessError):
        return "unavailable"
    return result.stdout.splitlines()[0].strip() if result.stdout else "unavailable"


def git_in(cwd: Path, *args: str) -> str:
    return command_line(["git", "-C", str(cwd), *args])


def compute_harness_fingerprint(root: Path) -> str:
    hasher = hashlib.sha256()
    bench_dir = root / "native" / "bench"
    if bench_dir.is_dir():
        for path in sorted(bench_dir.rglob("*")):
            if path.is_file():
                hasher.update(str(path.relative_to(root)).encode("utf-8"))
                hasher.update(path.read_bytes())
    tool_files = [
        "repeat_short_bench.py",
        "cold_start_bench.py",
        "allocation_bench.py",
        "perf_metadata.py",
        "run-perf.py",
        "perf_report.py",
        "build_wasm.sh",
        "build_wasm.ps1",
    ]
    for tf in tool_files:
        path = root / "scripts" / tf
        if path.is_file():
            hasher.update(str(path.relative_to(root)).encode("utf-8"))
            hasher.update(path.read_bytes())
    makefile = root / "Makefile"
    if makefile.is_file():
        hasher.update(b"Makefile")
        hasher.update(makefile.read_bytes())
    return hasher.hexdigest()[:16]


def cpu_name() -> str:
    cpuinfo = Path("/proc/cpuinfo")
    if cpuinfo.is_file():
        for line in cpuinfo.read_text(encoding="utf-8", errors="ignore").splitlines():
            if line.lower().startswith("model name") and ":" in line:
                return line.split(":", 1)[1].strip()
    return platform.processor() or "unknown"


def required_emscripten() -> str:
    path = Path("scripts/build_wasm.sh")
    if not path.is_file():
        return "unknown"
    match = re.search(r'^REQUIRED_EMSDK_VERSION="([^"]+)"', path.read_text(encoding="utf-8"), re.MULTILINE)
    return match.group(1) if match else "unknown"


def standard_library_version() -> str:
    try:
        result = subprocess.run(
            ["clang++", "-dM", "-E", "-x", "c++", "-include", "bits/c++config.h", "/dev/null"],
            check=True,
            capture_output=True,
            text=True,
        )
    except (OSError, subprocess.CalledProcessError):
        return "unavailable"
    match = re.search(r"^#define __GLIBCXX__ (\d+)$", result.stdout, re.MULTILINE)
    return match.group(1) if match else "unavailable"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--engine-dir", type=Path, default=None, help="directory containing engine sources")
    parser.add_argument("--engine-ref", default=None, help="Git ref or name of the engine")
    parser.add_argument("--harness-dir", type=Path, default=None, help="directory containing benchmark harness")
    args = parser.parse_args()

    harness_dir = (args.harness_dir or Path(os.environ.get("PERF_HARNESS_DIR", "") or Path.cwd())).resolve()
    engine_dir = (args.engine_dir or Path(os.environ.get("PERF_ENGINE_DIR", "") or harness_dir)).resolve()
    engine_ref = args.engine_ref or os.environ.get("PERF_ENGINE_REF", "")

    engine_commit_full = git_in(engine_dir, "rev-parse", "HEAD")
    engine_commit_short = git_in(engine_dir, "rev-parse", "--short", "HEAD")
    engine_branch = git_in(engine_dir, "branch", "--show-current")
    if not engine_ref:
        engine_ref = engine_branch if engine_branch and engine_branch != "unavailable" else engine_commit_short
    if not engine_branch or engine_branch == "unavailable":
        engine_branch = engine_ref or "detached"

    harness_commit_full = git_in(harness_dir, "rev-parse", "HEAD")
    harness_commit_short = git_in(harness_dir, "rev-parse", "--short", "HEAD")
    harness_branch = git_in(harness_dir, "branch", "--show-current")
    harness_fingerprint = compute_harness_fingerprint(harness_dir)

    status = subprocess.run(["git", "-C", str(harness_dir), "status", "--porcelain"], capture_output=True, text=True)
    git_state = "clean" if status.returncode == 0 and not status.stdout.strip() else "dirty"

    print(f"engine_ref={engine_ref}")
    print(f"engine_commit_full={engine_commit_full}")
    print(f"engine_commit_short={engine_commit_short}")
    print(f"harness_commit_full={harness_commit_full}")
    print(f"harness_commit_short={harness_commit_short}")
    print(f"harness_branch={harness_branch}")
    print(f"harness_fingerprint={harness_fingerprint}")
    print(f"harness_state={git_state}")
    print(f"commit_full={engine_commit_full}")
    print(f"commit_short={engine_commit_short}")
    print(f"branch={engine_branch}")
    print(f"date_utc={datetime.now(timezone.utc).isoformat(timespec='seconds')}")
    print(f"platform={platform.system()} {platform.release()} {platform.machine()}")
    print(f"cpu={cpu_name()}")
    print(f"clang={command_line(['clang++', '--version'])}")
    print(f"gcc={command_line(['g++', '--version'])}")
    print(f"cmake={command_line(['cmake', '--version'])}")
    print(f"emscripten_active={command_line(['em++', '--version']) if shutil.which('em++') else 'unavailable'}")
    print(f"emscripten_required={required_emscripten()}")
    print("cxx_standard=c++23")
    print("native_benchmark_flags=-std=c++23 -O3 -DNDEBUG")
    print("wasm_release_flags=-std=c++23 -Oz -fno-exceptions -fno-rtti -fno-stack-protector -ffunction-sections -fdata-sections -fno-math-errno -DNDEBUG")
    print(f"native_standard_library={command_line(['clang++', '-print-file-name=libstdc++.so'])}")
    print(f"libstdcxx_version={standard_library_version()}")
    try:
        affinity = ",".join(str(cpu) for cpu in sorted(os.sched_getaffinity(0)))
    except (AttributeError, OSError):
        affinity = "unavailable"
    print(f"available_cpu_affinity={affinity}")
    print(f"git_state={git_state}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
