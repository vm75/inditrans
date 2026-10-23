#!/usr/bin/env python3
"""Build reproducible libFuzzer seeds from the shared fixtures and malformed UTF-8."""

from __future__ import annotations

import argparse
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SCRIPTS = (
    "assamese", "bengali", "burmese", "devanagari", "gujarati", "gurmukhi", "kannada", "khmer",
    "malayalam", "oriya", "sinhala", "tamil", "tamil-extended", "telugu", "tibetan", "thai",
    "iast", "ipa", "iso", "itrans", "titus", "harvard-kyoto", "velthuis", "slp1", "wx",
    "readablelatin", "indic",
)
OPTIONS = {
    "TamilTraditional": 1,
    "TamilSuperscripted": 2,
    "ASCIINumerals": 4,
    "IgnoreVedicAccents": 8,
    "RetainSpecialMarkers": 16,
    "NoXMLTagHandling": 32,
    "DisableInference": 64,
}


def script_index(name: str) -> int:
    try:
        return SCRIPTS.index(name.lower())
    except ValueError:
        return 0


def option_bits(value: object) -> int:
    if not isinstance(value, str):
        return 0
    result = 0
    for name in value.replace("|", " ").replace(",", " ").split():
        result |= OPTIONS.get(name, 0)
    return result


def write_seed(directory: Path, name: str, source: str, target: str, text: str, options: int = 0) -> None:
    src = script_index(source)
    dst = script_index(target)
    if src == dst:
        dst = (dst + 1) % len(SCRIPTS)
    (directory / name).write_bytes(bytes((src, dst, options & 0x7F)) + text.encode("utf-8", errors="surrogatepass"))


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    output = args.output
    for name in ("transliterate", "preservation", "utf8", "marker_filter", "tamil_marker", "c_abi"):
        (output / name).mkdir(parents=True, exist_ok=True)

    sequence = 0
    tamil_sequence = 0

    def add_tamil_seed(value: object) -> None:
        nonlocal tamil_sequence
        if not isinstance(value, str):
            return
        if not any("\u0b80" <= char <= "\u0bff" or char in "¹²³⁴₁₂₃₄" for char in value):
            return
        (output / "tamil_marker" / f"fixture-{tamil_sequence:05d}").write_bytes(value.encode("utf-8", errors="surrogatepass"))
        tamil_sequence += 1

    for fixture in (ROOT / "test-files/test-cases.json", ROOT / "test-files/sanscript-tests.json"):
        cases = json.loads(fixture.read_text(encoding="utf-8"))
        for case in cases:
            text = case.get("text")
            source = case.get("script")
            if isinstance(text, str) and isinstance(source, str):
                add_tamil_seed(text)
                for target in case.get("targets", []):
                    if not isinstance(target, dict) or not isinstance(target.get("script"), str):
                        continue
                    add_tamil_seed(target.get("text"))
                    seed = f"fixture-{sequence:05d}"
                    write_seed(output / "transliterate", seed, source, target["script"], text, option_bits(target.get("options")))
                    write_seed(output / "preservation", seed, source, target["script"], text, option_bits(target.get("options")))
                    (output / "utf8" / seed).write_bytes(text.encode("utf-8", errors="surrogatepass"))
                    sequence += 1
            elif case.get("type") == "any-to-any":
                targets = case.get("targets", [])
                for source_entry in targets:
                    if not isinstance(source_entry, dict) or not isinstance(source_entry.get("script"), str):
                        continue
                    add_tamil_seed(source_entry.get("text"))
                    for target_entry in targets:
                        if not isinstance(target_entry, dict) or not isinstance(target_entry.get("script"), str):
                            continue
                        add_tamil_seed(target_entry.get("text"))
                        if source_entry["script"].lower() == target_entry["script"].lower():
                            continue
                        seed = f"fixture-{sequence:05d}"
                        write_seed(output / "transliterate", seed, source_entry["script"], target_entry["script"], source_entry.get("text", ""))
                        write_seed(output / "preservation", seed, source_entry["script"], target_entry["script"], source_entry.get("text", ""))
                        (output / "utf8" / seed).write_bytes(source_entry.get("text", "").encode("utf-8", errors="surrogatepass"))
                        sequence += 1

    malformed = {
        "truncated-2-byte": b"\xc2",
        "truncated-3-byte": b"\xe2",
        "truncated-3-byte-two": b"\xe2\x80",
        "truncated-4-byte": b"\xf0\x9f\x92",
        "stray-continuation": b"\x80",
        "overlong": b"\xc0\xaf",
        "surrogate": b"\xed\xa0\x80",
        "above-unicode": b"\xf4\x90\x80\x80",
        "embedded-nul": b"a\x00b",
    }
    for name, payload in malformed.items():
        (output / "utf8" / name).write_bytes(payload)
        (output / "marker_filter" / name).write_bytes(payload)
        (output / "transliterate" / name).write_bytes(bytes((3, 11, 16)) + payload)
        (output / "preservation" / name).write_bytes(bytes((3, 11, 16)) + payload)
        (output / "tamil_marker" / name).write_bytes(payload)
        (output / "c_abi" / name).write_bytes(bytes((0,)) + payload)

    (output / "marker_filter" / "marker-only").write_bytes("¹²³⁴₁₂₃₄ˮˇʽʼ".encode("utf-8"))
    (output / "tamil_marker" / "marker-only").write_bytes("¹²³⁴₁₂₃₄".encode("utf-8"))
    (output / "tamil_marker" / "malformed-prefix-marker").write_bytes(b"\xe2x" + "ʼ".encode("utf-8"))
    for mask in (1, 2, 4, 8, 16, 32, 63):
        (output / "c_abi" / f"null-mask-{mask:02x}").write_bytes(bytes((mask,)) + b"\0" * 8 + b"namaste")
    (output / "c_abi" / "invalid-options").write_bytes(b"\0" + b"\xff" * 8 + b"namaste")
    (output / "c_abi" / "valid-option-mask").write_bytes(b"\0\x7f" + b"\0" * 7 + b"namaste")
    (output / "c_abi" / "unknown-script").write_bytes(bytes((27,)) + b"\0" * 8 + b"namaste")
    print(f"Wrote {sequence} transliteration cases, {tamil_sequence} Tamil marker cases, and "
          f"{len(malformed)} malformed byte seeds to {output}")


if __name__ == "__main__":
    main()
