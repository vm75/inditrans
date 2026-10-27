#!/usr/bin/env python3
"""Bundle the canonical native engine inside the Flutter package before publishing."""

import argparse
from pathlib import Path
import shutil


def prepare_package(root: Path) -> None:
    source = root / "native" / "src"
    destination = root / "flutter" / "src"
    if not (source / "CMakeLists.txt").is_file() or not (source / "inditrans.cpp").is_file():
        raise FileNotFoundError(f"Native engine sources missing from {source}")
    # Replace the generated copy so removed headers cannot leak into a release.
    if destination.exists():
        shutil.rmtree(destination)
    shutil.copytree(source, destination, ignore=shutil.ignore_patterns("build*", "__pycache__"))
    shutil.copyfile(root / "CHANGELOG.md", root / "flutter" / "CHANGELOG.md")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, help="also copy the package to a new isolated directory")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    prepare_package(root)
    if args.output:
        shutil.copytree(root / "flutter", args.output,
                        ignore=shutil.ignore_patterns(".dart_tool", "build", ".gradle", ".cxx",
                                                     "ephemeral", "Pods", ".symlinks", "local.properties"))
    print("Prepared flutter/src and flutter/CHANGELOG.md for publication")
