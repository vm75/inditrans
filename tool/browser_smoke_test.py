#!/usr/bin/env python3
"""Run the shared browser fixture in headless Chromium and require all cases pass."""

from __future__ import annotations

import argparse
import re
import shutil
import subprocess
import tempfile
from pathlib import Path


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--browser", default="chromium")
    args = parser.parse_args()
    browser = shutil.which(args.browser)
    if not browser:
        raise SystemExit(f"Browser executable not found: {args.browser}")

    root = Path(__file__).resolve().parents[1]
    fixture = (root / "js/tests/test.html").as_uri()
    with tempfile.TemporaryDirectory(prefix="inditrans-chrome-") as profile:
        result = subprocess.run(
            [
                browser,
                "--headless=new",
                "--no-sandbox",
                "--disable-dev-shm-usage",
                "--disable-gpu",
                "--no-first-run",
                f"--user-data-dir={profile}",
                "--allow-file-access-from-files",
                "--virtual-time-budget=10000",
                "--dump-dom",
                fixture,
            ],
            capture_output=True,
            text=True,
            timeout=45,
        )
    if result.returncode != 0:
        raise SystemExit(f"Chromium exited {result.returncode}: {result.stderr[-2000:]}")

    match = re.search(r'<div id="test-results"([^>]*)>', result.stdout)
    if not match:
        raise SystemExit("Browser fixture did not render its result element")
    attrs = match.group(1)
    status = re.search(r'data-status="([^"]+)"', attrs)
    count = re.search(r'data-count="([0-9]+)"', attrs)
    failures = re.search(r'data-failures="([0-9]+)"', attrs)
    if not status or status.group(1) != "passed" or not count or not failures:
        raise SystemExit(f"Browser tests did not pass. Result: {attrs}\n{result.stderr[-2000:]}")
    if int(count.group(1)) == 0 or int(failures.group(1)) != 0:
        raise SystemExit(f"Invalid browser test counts: {attrs}")
    print(f"Browser shared suite passed: {count.group(1)} conversions, each repeated twice")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
