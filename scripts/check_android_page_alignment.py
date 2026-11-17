#!/usr/bin/env python3
"""Check 16 KiB ELF and ZIP alignment of Android 64-bit native libraries.

Usage: python3 scripts/check_android_page_alignment.py app-release.apk [app-release.aab]

AAB libraries are checked for ELF alignment; use bundletool separately to
inspect the AAB's PAGE_ALIGNMENT_16K packaging setting.
"""

import argparse
from pathlib import Path
import struct
import sys
import zipfile

PAGE_SIZE = 16 * 1024


def elf_load_alignments(data: bytes) -> list[int]:
    """Return the PT_LOAD p_align values of a 64-bit ELF shared library."""
    if len(data) < 64 or data[:4] != b"\x7fELF" or data[4] != 2:
        raise ValueError("expected a 64-bit ELF file")
    endian = {1: "<", 2: ">"}.get(data[5])
    if endian is None:
        raise ValueError("invalid ELF byte order")
    phoff = struct.unpack_from(endian + "Q", data, 32)[0]
    phentsize, phnum = struct.unpack_from(endian + "HH", data, 54)
    if phentsize < 56 or not phnum or phoff + phentsize * phnum > len(data):
        raise ValueError("invalid ELF program header table")
    alignments = []
    for i in range(phnum):
        offset = phoff + i * phentsize
        if struct.unpack_from(endian + "I", data, offset)[0] == 1:  # PT_LOAD
            alignments.append(struct.unpack_from(endian + "Q", data, offset + 48)[0])
    if not alignments:
        raise ValueError("ELF has no PT_LOAD segments")
    return alignments


def apk_data_offset(apk, member: zipfile.ZipInfo) -> int:
    """Calculate member data position from ZIP's local header."""
    apk.seek(member.header_offset)
    header = apk.read(30)
    if len(header) != 30 or struct.unpack_from("<I", header)[0] != 0x04034B50:
        raise ValueError("invalid ZIP local file header")
    filename_len, extra_len = struct.unpack_from("<HH", header, 26)
    return member.header_offset + 30 + filename_len + extra_len


def check_archive(path: Path) -> tuple[int, list[str]]:
    """Return (count of 64-bit libraries checked, problems)."""
    problems = []
    checked = 0
    is_apk = path.suffix.lower() == ".apk"
    with zipfile.ZipFile(path) as archive, path.open("rb") as raw:
        for member in archive.infolist():
            name = member.filename
            if not name.endswith(".so") or not (
                "/arm64-v8a/" in f"/{name}" or "/x86_64/" in f"/{name}"
            ):
                continue
            checked += 1
            try:
                alignments = elf_load_alignments(archive.read(member))
                if any(a < PAGE_SIZE or a % PAGE_SIZE for a in alignments):
                    values = ", ".join(hex(a) for a in alignments)
                    problems.append(f"{name}: PT_LOAD alignment(s) {values}; need >= 0x4000")
                if is_apk and member.compress_type == zipfile.ZIP_STORED:
                    offset = apk_data_offset(raw, member)
                    if offset % PAGE_SIZE:
                        problems.append(
                            f"{name}: uncompressed APK entry starts at {offset:#x}; "
                            "need 16 KiB ZIP alignment"
                        )
            except (ValueError, struct.error, zipfile.BadZipFile) as error:
                problems.append(f"{name}: {error}")
    if not checked:
        problems.append("no arm64-v8a or x86_64 shared libraries found")
    return checked, problems


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("archives", nargs="+", type=Path, help="Android APK or AAB files")
    args = parser.parse_args(argv)
    failed = False
    for path in args.archives:
        if path.suffix.lower() not in (".apk", ".aab"):
            parser.error(f"unsupported artifact (expected APK or AAB): {path}")
        try:
            count, problems = check_archive(path)
        except (OSError, zipfile.BadZipFile) as error:
            count, problems = 0, [str(error)]
        print(f"{path}: checked {count} 64-bit shared libraries")
        for problem in problems:
            print(f"  FAIL: {problem}")
        if not problems:
            print("  PASS: 16 KiB ELF segment alignment" +
                  (" and uncompressed APK ZIP alignment" if path.suffix.lower() == ".apk" else ""))
        failed |= bool(problems)
    return int(failed)


if __name__ == "__main__":
    sys.exit(main())
