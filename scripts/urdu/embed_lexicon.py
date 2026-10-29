#!/usr/bin/env python3
"""Deterministically embed the optional IUL1 Urdu corpus into the core library.

Pronunciation dataset: Wiktionary contributors (CC BY-SA 4.0).
See scripts/urdu/README.md for copyright, changes and source attribution.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import zlib

ROOT = Path(__file__).resolve().parents[2]
REPORT = ROOT / "scripts/urdu/wordlist-report.json"
SOURCE = ROOT / "demo/urdu-pronunciation.iul"
DESTINATION = ROOT / "native/src/urdu_embedded_pack.h"


def generate(raw, *, expected_sha256):
    if len(raw) < 16 or raw[:4] != b"IUL1":
        raise ValueError("not a valid-size IUL1 asset")
    if hashlib.sha256(raw).hexdigest() != expected_sha256:
        raise ValueError("embedded Urdu source does not match pinned SHA-256")
    if struct.unpack_from("<I", raw, 4)[0] != zlib.crc32(raw[8:]):
        raise ValueError("IUL1 CRC mismatch")
    chunks = ["    \"" + "".join(f"\\x{byte:02x}" for byte in raw[i:i + 32]) + "\""
              for i in range(0, len(raw), 32)]
    return (
        "// Generated from demo/urdu-pronunciation.iul by scripts/urdu/embed_lexicon.py.\n"
        "// Data license and attribution: scripts/urdu/wordlist-report.json.\n"
        "// Attribution, sources and license: scripts/urdu/README.md.\n"
        "#pragma once\n\n"
        "namespace inditrans::embedded_urdu {\n"
        "inline constexpr char pack[] =\n"
        + "\n".join(chunks) + ";\n"
        "inline constexpr unsigned long pack_size = sizeof(pack) - 1;\n"
        + f"static_assert(pack_size == {len(raw)});\n"
        "} // namespace inditrans::embedded_urdu\n"
    )


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, default=SOURCE)
    parser.add_argument("--output", type=Path, default=DESTINATION)
    parser.add_argument("--report", type=Path, default=REPORT, help="manifest containing expected pack_sha256 and license")
    parser.add_argument("--check", action="store_true", help="verify checked-in generated header")
    args = parser.parse_args()
    report = json.loads(args.report.read_text(encoding="utf-8"))
    digest = report.get("pack_sha256")
    if not isinstance(digest, str) or len(digest) != 64:
        parser.error("manifest must include a 64-character pack_sha256")
    result = generate(args.source.read_bytes(), expected_sha256=digest)
    if args.check:
        if args.output.read_text(encoding="utf-8") != result:
            parser.error("generated Urdu header is stale; run scripts/urdu/embed_lexicon.py")
    else:
        args.output.write_text(result, encoding="utf-8")
    print(f"Embedded {args.source.stat().st_size} IUL1 bytes; check={args.check}")


if __name__ == "__main__":
    main()
