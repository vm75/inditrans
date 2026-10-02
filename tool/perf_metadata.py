#!/usr/bin/env python3
"""Emit reproducibility metadata for a local performance snapshot."""

from __future__ import annotations

import platform
import re
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


def required_emscripten() -> str:
    path = Path("tool/build_wasm.sh")
    if not path.is_file():
        return "unknown"
    match = re.search(r'^REQUIRED_EMSDK_VERSION="([^"]+)"', path.read_text(encoding="utf-8"), re.MULTILINE)
    return match.group(1) if match else "unknown"


def main() -> int:
    status = subprocess.run(["git", "status", "--porcelain"], capture_output=True, text=True)
    git_state = "clean" if status.returncode == 0 and not status.stdout.strip() else "dirty"

    print(f"commit={git('rev-parse', '--short', 'HEAD')}")
    print(f"platform={platform.system()} {platform.release()} {platform.machine()}")
    print(f"cpu={platform.processor() or 'unknown'}")
    print(f"clang={command_line(['clang++', '--version'])}")
    print(f"emscripten_active={command_line(['em++', '--version']) if shutil.which('em++') else 'unavailable'}")
    print(f"emscripten_required={required_emscripten()}")
    print(f"git_state={git_state}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
