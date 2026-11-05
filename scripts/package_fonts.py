#!/usr/bin/env python3
"""Package the checked-in fonts and client instructions for a tagged release."""

import argparse
from pathlib import Path
import re
from zipfile import ZIP_DEFLATED, ZipFile, ZipInfo

ROOT = Path(__file__).resolve().parents[1]


def package_fonts(root: Path, tag: str, output: Path) -> Path:
    if not re.fullmatch(r"v\d+\.\d+\.\d+", tag):
        raise ValueError("Expected a release tag vMAJOR.MINOR.PATCH")
    fonts = root / "fonts"
    readme, heading, _ = (fonts / "README.md").read_text(encoding="utf-8").partition(
        "\n## Sources and generation\n"
    )
    if not heading:
        raise ValueError("Could not find client instructions in fonts/README.md")
    files = {
        name: (fonts / name).read_bytes()
        for name in ["inditrans-indic-fallback.ttf", "inditrans-indic-fallback.woff2", "OFL.txt"]
    }
    files["README.md"] = (readme.rstrip() + f"\n\nInditrans release: {tag}.\n").encode("utf-8")
    if any(not content for content in files.values()):
        raise ValueError("Font release files must not be empty")
    output.mkdir(parents=True, exist_ok=True)
    destination = output / f"inditrans-fonts-{tag}.zip"
    with ZipFile(destination, "w") as archive:
        for name, content in files.items():
            # Fixed timestamps and permissions make packaging independent of checkout metadata.
            info = ZipInfo(name, date_time=(1980, 1, 1, 0, 0, 0))
            info.create_system = 3
            info.external_attr = 0o100644 << 16
            info.compress_type = ZIP_DEFLATED
            archive.writestr(info, content)
    return destination


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--tag", required=True)
    parser.add_argument("--output", type=Path, default=ROOT / "out")
    args = parser.parse_args()
    print(package_fonts(ROOT, args.tag, args.output))


if __name__ == "__main__":
    main()
