#!/usr/bin/env python3
"""Build a deterministic, versioned Urdu pronunciation lexicon from reviewed data.

This is an offline data preparation tool, not a runtime dependency. Input readings
are sequences of canonical Inditrans token units; see README.md in this directory.
"""

import argparse
import hashlib
import json
import pathlib
import struct
import unicodedata
import zlib

ROOT = pathlib.Path(__file__).resolve().parents[2]
ALIASES = str.maketrans({"ك": "ک", "ي": "ی", "ى": "ی", "ه": "ہ"})


def normalize(word):
    # NFC retains vowel and other meaningful marks. Do not strip or guess them.
    return unicodedata.normalize("NFC", word.translate(ALIASES))


def build(source, tables):
    if source.get("format") != 1 or not isinstance(source.get("version"), str) or not source["version"] or not source.get("provenance"):
        raise ValueError("format, version and provenance are required")
    script = tables["roman"]["iast"]
    limits = {"v": len(script["vowels"]), "c": len(script["consonants"]),
              "m": len(script["vowelMarks"]), "o": len(script["otherDiacritics"])}
    entries = {}
    for entry in source["entries"]:
        word = normalize(entry["word"])
        if not word or unicodedata.category(word[0])[0] != "L" or any(
            not ("\u0600" <= ch <= "\u06ff" or "\u0750" <= ch <= "\u077f")
            or unicodedata.category(ch)[0] not in "LM" for ch in word
        ):
            raise ValueError(f"invalid Urdu word: {word!r}")
        readings = entry["readings"]
        if not readings:
            raise ValueError(f"no readings for {word}")
        checked = []
        for reading in readings:
            if not reading:
                raise ValueError(f"empty reading for {word}")
            units = []
            for unit in reading:
                if set(unit) - {"v", "c", "m", "o"} or ("v" in unit) == ("c" in unit):
                    raise ValueError(f"invalid unit in {word}")
                if "m" in unit and "c" not in unit:
                    raise ValueError(f"vowel mark without consonant in {word}")
                for key, index in unit.items():
                    if type(index) is not int or not 0 <= index < limits[key]:
                        raise ValueError(f"invalid {key} index in {word}")
                units.append({key: unit[key] for key in ("v", "c", "m", "o") if key in unit})
            if units in checked:
                raise ValueError(f"duplicate reading for {word}")
            checked.append(units)
        if word in entries:
            raise ValueError(f"duplicate normalized spelling: {word}")
        entries[word] = checked
    return {"format": 1, "version": source["version"], "provenance": source["provenance"],
            "entries": [{"word": word, "readings": entries[word]} for word in sorted(entries)]}


def binary_pack(lexicon):
    """Encode portable v1 records, retaining alternate readings in source order."""
    out = bytearray()
    metadata = json.dumps({key: lexicon[key] for key in ("version", "provenance", "source_sha256")},
                          ensure_ascii=False, separators=(",", ":")).encode("utf-8")
    out.extend(struct.pack("<I", len(metadata)))
    out.extend(metadata)
    out.extend(struct.pack("<I", len(lexicon["entries"])))
    for entry in lexicon["entries"]:
        word = entry["word"].encode("utf-8")
        readings = entry["readings"]
        if len(word) > 65535 or len(readings) > 65535:
            raise ValueError("word or reading count exceeds pack limits")
        out.extend(struct.pack("<HH", len(word), len(readings)))
        out.extend(word)
        for reading in readings:
            if len(reading) > 65535:
                raise ValueError("reading exceeds pack limits")
            out.extend(struct.pack("<H", len(reading)))
            for unit in reading:
                lead = "c" if "c" in unit else "v"
                out.extend((1 if lead == "c" else 0, unit[lead], unit.get("m", 255), unit.get("o", 255)))
    return b"IUL1" + struct.pack("<I", zlib.crc32(out)) + out


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=pathlib.Path)
    parser.add_argument("output", type=pathlib.Path)
    parser.add_argument("--binary", action="store_true", help="emit the portable IUL1 binary pack")
    args = parser.parse_args()
    source_bytes = args.source.read_bytes()
    result = build(json.loads(source_bytes), json.loads((ROOT / "scripts/script_data.json").read_text()))
    result["source_sha256"] = hashlib.sha256(source_bytes).hexdigest()
    if args.binary:
        args.output.write_bytes(binary_pack(result))
    else:
        args.output.write_text(json.dumps(result, ensure_ascii=False, separators=(",", ":")) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
