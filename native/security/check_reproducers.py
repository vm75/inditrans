#!/usr/bin/env python3
"""Run the fixed security reproducers as positive sanitizer regressions."""

from __future__ import annotations

import os
import subprocess
import sys

MODES = (
    "truncated-marker",
    "output-alias",
    "invalid-options",
    "null-text",
    "null-from",
    "null-to",
    "null-skip-start",
    "null-skip-end",
    "null-script-query",
    "flutter-release",
    "builder-replace-short",
    "builder-realloc-failure",
    "builder-realloc-failure-after-growth",
    "translit-buffer-realloc-failure",
    "c-abi-realloc-failure",
    "default-empty-view",
)


def main() -> int:
    if len(sys.argv) != 2:
        print(f"usage: {sys.argv[0]} REPRODUCER_BINARY", file=sys.stderr)
        return 2
    env = os.environ.copy()
    env["ASAN_OPTIONS"] = "detect_leaks=0:halt_on_error=1"
    env["UBSAN_OPTIONS"] = "halt_on_error=1:print_stacktrace=1"
    failed = False
    for mode in MODES:
        result = subprocess.run([sys.argv[1], mode], text=True, capture_output=True, env=env)
        diagnostic = result.stdout + result.stderr
        if result.returncode != 0 or "ERROR: AddressSanitizer" in diagnostic or "runtime error:" in diagnostic:
            failed = True
            print(f"FAIL {mode}: expected safe completion (exit {result.returncode})")
            print(diagnostic[-3000:])
        else:
            print(f"PASS {mode}")

    leak_env = env.copy()
    leak_env["ASAN_OPTIONS"] = "detect_leaks=1:halt_on_error=1"
    leak_env["LSAN_OPTIONS"] = "exitcode=23"
    result = subprocess.run([sys.argv[1], "flutter-release"], text=True, capture_output=True, env=leak_env)
    diagnostic = result.stdout + result.stderr
    if "LeakSanitizer has encountered a fatal error" in diagnostic:
        print("SKIP flutter-release leak check: LeakSanitizer is unavailable under this runtime's ptrace restrictions")
    elif result.returncode != 0 or "LeakSanitizer: detected memory leaks" in diagnostic:
        failed = True
        print(f"FAIL flutter-release leak check (exit {result.returncode})")
        print(diagnostic[-3000:])
    else:
        print("PASS flutter-release leak check")
    return int(failed)


if __name__ == "__main__":
    raise SystemExit(main())
