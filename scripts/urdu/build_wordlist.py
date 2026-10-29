#!/usr/bin/env python3
"""Compile a source-neutral Urdu wordlist into deterministic IUL1 pronunciation data.

Input TSV columns: Urdu spelling, pronunciation, notation, priority.
Notation is 'iast' (e.g. kitāb or pal) or 'ipa' (space-separated IPA tokens
accepted by import_zuhri.units). Priority is an OPTIONAL editorial default
preference, not an empirical frequency or a context model. Highest priority
wins; ties use a stable canonical token ordering. All distinct supported
readings are retained in IUL1, even though the current reader uses only #1.

The caller is responsible for the provenance and redistribution rights of
input wordlists. Generating a different pack does not remove upstream terms.
"""
import argparse
import collections
import csv
import hashlib
import io
import json
from pathlib import Path
import sys
import unicodedata

from build_lexicon import ROOT, binary_pack, build, normalize
from import_zuhri import eligible_word, units as ipa_units

TABLES = json.loads((ROOT / "scripts/script_data.json").read_text(encoding="utf-8"))
IAST = TABLES["roman"]["iast"]

# Extended variants are not a complete Hindi/Urdu G2P. Only known canonical
# spellings are accepted; the caller can use canonical phoneme JSON instead.
VOWELS = {v: i for i, v in enumerate(IAST["vowels"]) if i < 19}
CONSONANTS = {c: i for i, c in enumerate(IAST["consonants"]) if i < 45}
VOWELS.update({"a": 0, "ā": 1, "i": 2, "ī": 3, "u": 4, "ū": 5})
OTHER = {"ṃ": 1, "m̐": 0}
SYMBOLS = sorted(((v, "v", i) for v, i in VOWELS.items()), key=lambda x: (-len(x[0]), x[0]))
SYMBOLS += sorted(((c, "c", i) for c, i in CONSONANTS.items()), key=lambda x: (-len(x[0]), x[0]))
SYMBOLS = sorted(SYMBOLS, key=lambda x: (-len(x[0]), x[0], x[1]))


def roman_units(reading):
    reading = unicodedata.normalize("NFC", reading.strip())
    if not reading or any(ch.isspace() for ch in reading):
        raise ValueError("empty or multiword IAST reading")
    out = []
    offset = 0
    while offset < len(reading):
        if reading.startswith("ṃ", offset) and out:
            out[-1]["o"] = 1
            offset += 1
            continue
        match = next((item for item in SYMBOLS if reading.startswith(item[0], offset)), None)
        if match is None:
            raise ValueError("unsupported IAST symbol " + repr(reading[offset:offset + 3]))
        symbol, category, index = match
        offset += len(symbol)
        if category == "c":
            out.append({"c": index, "m": 0})
        elif out and "c" in out[-1] and out[-1].get("m") == 0:
            if index == 0:
                del out[-1]["m"]
            else:
                out[-1]["m"] = index
        else:
            out.append({"v": index})
    return out


def sha(data):
    return hashlib.sha256(data).hexdigest()


def read_tsv(raw, source_label, *, strict=False):
    """Yield (word, canonical reading, editorial priority), plus counts."""
    rows = csv.reader(io.StringIO(raw.decode("utf-8-sig")), delimiter="\t")
    parsed = []
    rejected = collections.Counter()
    unresolved = set()
    for line_number, fields in enumerate(rows, 1):
        if not fields or not fields[0].strip() or fields[0].lstrip().startswith("#"):
            continue
        if len(fields) not in (2, 3, 4):
            raise ValueError(f"{source_label}:{line_number}: expected 2-4 TSV columns")
        word = normalize(fields[0].strip())
        if not eligible_word(word):
            rejected["invalid spelling"] += 1
            continue
        spelling = fields[1].strip()
        if not spelling:
            unresolved.add(word)
            continue
        notation = fields[2].strip().lower() if len(fields) >= 3 else "iast"
        if notation not in {"iast", "ipa"}:
            raise ValueError(f"{source_label}:{line_number}: unsupported notation {notation!r}")
        try:
            priority = int(fields[3].strip()) if len(fields) == 4 else 0
        except ValueError as exc:
            raise ValueError(f"{source_label}:{line_number}: priority must be an integer") from exc
        try:
            reading = roman_units(spelling) if notation == "iast" else ipa_units(spelling)
        except ValueError as exc:
            if strict:
                raise ValueError(f"{source_label}:{line_number}: {exc}") from exc
            rejected["unsupported pronunciation"] += 1
            unresolved.add(word)
            continue
        parsed.append((word, reading, priority))
    return parsed, rejected, unresolved


def compile_wordlist(sources, *, version, provenance, license_name, selection=None, limit=None,
                     strict=False):
    if not sources or not version or not provenance or not license_name:
        raise ValueError("non-empty inputs, version, provenance and license are required")
    by_word = collections.defaultdict(dict)
    rejected = collections.Counter()
    unresolved = set()
    digests = {}
    for label, raw in sources:
        parsed, exclusions, pending = read_tsv(raw, label, strict=strict)
        digests[label] = sha(raw)
        rejected.update(exclusions)
        unresolved.update(pending)
        for word, reading, priority in parsed:
            key = json.dumps(reading, ensure_ascii=False, sort_keys=True, separators=(",", ":"))
            # Repeated identical readings do not accidentally multiply priority.
            old = by_word[word].get(key)
            if old is None or priority > old[0]:
                by_word[word][key] = (priority, reading)

    available = set(by_word)
    if selection is not None:
        selected = {normalize(x.strip()) for x in selection.decode("utf-8-sig").splitlines()
                    if x.strip() and not x.lstrip().startswith("#")}
        unknown = selected - available
        if unknown:
            raise ValueError(f"{len(unknown)} selected words have no valid reading; examples: {sorted(unknown)[:5]}")
        by_word = {word: by_word[word] for word in selected}
    if limit is not None:
        if limit < 1:
            raise ValueError("limit must be positive")
        ranked = sorted(by_word, key=lambda w: (-max(v[0] for v in by_word[w].values()), w))
        by_word = {word: by_word[word] for word in ranked[:limit]}

    entries = []
    ambiguous = 0
    for word in sorted(by_word):
        ranked = sorted(by_word[word].items(), key=lambda kv: (-kv[1][0], kv[0]))
        readings = [value[1] for _, value in ranked]
        ambiguous += len(readings) > 1
        entries.append({"word": word, "readings": readings})

    source = {"format": 1, "version": version, "provenance": provenance + "; license: " + license_name,
              "entries": entries}
    canonical = build(source, TABLES)
    source_bytes = (json.dumps(source, ensure_ascii=False, sort_keys=True,
                               separators=(",", ":")) + "\n").encode("utf-8")
    canonical["source_sha256"] = sha(source_bytes)
    packed = binary_pack(canonical)
    result = {
        "format": 1, "version": version, "license": license_name, "provenance": provenance,
        "source_sha256": digests,
        "selection_sha256": sha(selection) if selection is not None else None,
        "candidate_words": len(available),
        "entries": len(entries),
        "ambiguities_with_alternatives": ambiguous,
        "unresolved_words": len(unresolved - available),
        "rejected": dict(sorted(rejected.items())),
        "pack_bytes": len(packed), "pack_sha256": sha(packed),
        "selection_policy": "Highest explicit editorial priority then lexicographically sorted canonical units; no context",
        "limitations": "Wordlist pronunciations may be unverified. Priority is not measured frequency. "
                       "Words absent from pack fall back to literal transliteration.",
    }
    return packed, result, canonical


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("wordlists", type=Path, nargs="+", help="TSV wordlists with Urdu/IAST or IPA pronunciations")
    p.add_argument("--out", type=Path, required=True, help="output IUL1 binary")
    p.add_argument("--report", type=Path, required=True, help="output JSON provenance/counts/digests")
    p.add_argument("--canonical-json", type=Path, help="optional all-readings JSON")
    p.add_argument("--version", required=True)
    p.add_argument("--provenance", required=True)
    p.add_argument("--license", required=True, dest="license_name")
    p.add_argument("--selection", type=Path, help="optional one-Urdu-word-per-line shortlist")
    p.add_argument("--limit", type=int, help="optional entry count cap for later curation; not a byte limit")
    p.add_argument("--strict", action="store_true", help="fail rather than skip an unsupported pronunciation")
    p.add_argument("--check", action="store_true", help="fail if checked-in pack/report differ from source wordlists")
    args = p.parse_args()
    sources = [(str(path), path.read_bytes()) for path in args.wordlists]
    selection = args.selection.read_bytes() if args.selection else None
    pack, report, canonical = compile_wordlist(
        sources, version=args.version, provenance=args.provenance,
        license_name=args.license_name, selection=selection, limit=args.limit, strict=args.strict)
    report_json = json.dumps(report, ensure_ascii=False, sort_keys=True, indent=2) + "\n"
    canonical_json = json.dumps(canonical, ensure_ascii=False, separators=(",", ":")) + "\n"
    if args.check:
        if args.out.read_bytes() != pack or args.report.read_text(encoding="utf-8") != report_json:
            p.error("generated Urdu pack/report are stale; rebuild from the current wordlists")
        if args.canonical_json and args.canonical_json.read_text(encoding="utf-8") != canonical_json:
            p.error("generated canonical Urdu source is stale")
    else:
        for path in (args.out, args.report, args.canonical_json):
            if path:
                path.parent.mkdir(parents=True, exist_ok=True)
        args.out.write_bytes(pack)
        args.report.write_text(report_json, encoding="utf-8")
        if args.canonical_json:
            args.canonical_json.write_text(canonical_json, encoding="utf-8")
    print(f"Urdu wordlist: {report['entries']} pronunciations, {report['ambiguities_with_alternatives']} ambiguous, "
          f"{report['pack_bytes']} packed bytes; SHA-256 {report['pack_sha256']}")


if __name__ == "__main__":
    main()
