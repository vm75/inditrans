#!/usr/bin/env python3
"""Emit reproducibility metadata for a local performance snapshot."""

from __future__ import annotations

import platform
import re
import os
from datetime import datetime, timezone
import shutil
import subprocess
from pathlib import Path


def command_line(command: list[str]) -> str:
    try:
        result = subprocess.run(command, check=True, capture_output=True, text=True)
    except (OSError, subprocess.CalledProcessError):
        return "unavailable"
    return result.stdout.splitlines()[0].strip() if result.stdout else "unavailable"


def git(*args: str) -> str:
    return command_line(["git", *args])


def cpu_name() -> str:
    cpuinfo = Path("/proc/cpuinfo")
    if cpuinfo.is_file():
        for line in cpuinfo.read_text(encoding="utf-8", errors="ignore").splitlines():
            if line.lower().startswith("model name") and ":" in line:
                return line.split(":", 1)[1].strip()
    return platform.processor() or "unknown"


def required_emscripten() -> str:
    path = Path("tool/build_wasm.sh")
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
    status = subprocess.run(["git", "status", "--porcelain"], capture_output=True, text=True)
    git_state = "clean" if status.returncode == 0 and not status.stdout.strip() else "dirty"

    print(f"commit_full={git('rev-parse', 'HEAD')}")
    print(f"commit_short={git('rev-parse', '--short', 'HEAD')}")
    print(f"branch={git('branch', '--show-current')}")
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
