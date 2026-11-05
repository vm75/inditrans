#!/usr/bin/env python3
"""Stage a standalone Inditrans demo with engine assets from the same checkout.

The Pages workflow runs this script from trusted main, while --source can point
at a PR checkout. Read files from the source, but never execute code from it
during the deployment job.
"""

from __future__ import annotations

import argparse
import re
import shutil
from pathlib import Path

_VERSION = re.compile(r"(?m)^version:\s*(\d+\.\d+\.\d+)\s*$")
_META = re.compile(r'(<meta name="inditrans-version" content=")[^"]+("/?>)')


def stage_demo(source: Path, destination: Path) -> str:
    """Stage the demo and matching checked-in JS assets; return version string."""
    source = source.resolve()
    destination = destination.resolve()
    manifest = source / "flutter/pubspec.yaml"
    match = _VERSION.search(manifest.read_text(encoding="utf-8"))
    if not match:
        raise ValueError(f"Could not read package version from {manifest}")
    version = match.group(1)

    shutil.copytree(source / "demo", destination / "demo")
    # Older PR checkouts keep fonts inside demo/; newer ones share root fonts/.
    if (source / "fonts").is_dir():
        shutil.copytree(source / "fonts", destination / "fonts")
    (destination / "js/public").mkdir(parents=True, exist_ok=True)
    shutil.copy2(source / "js/public/inditrans.js", destination / "js/public/inditrans.js")
    shutil.copy2(source / "js/inditrans-standalone.js", destination / "js/inditrans-standalone.js")

    page = destination / "demo/index.html"
    original = page.read_text(encoding="utf-8")
    html, changes = _META.subn(lambda m: f"{m.group(1)}v{version}{m.group(2)}", original, count=1)
    if changes != 1:
        raise ValueError(f"Expected one inditrans-version meta tag in {page}")
    page.write_text(html, encoding="utf-8")
    (destination / "index.html").write_text(
        '<!doctype html><meta http-equiv="refresh" content="0;url=demo/">'
        '<a href="demo/">Inditrans demo</a>\n',
        encoding="utf-8",
    )
    return version


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--destination", type=Path, required=True)
    args = parser.parse_args()
    version = stage_demo(args.source, args.destination)
    print(f"Staged {args.source} at {args.destination} (engine v{version})")


if __name__ == "__main__":
    main()
