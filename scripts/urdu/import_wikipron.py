#!/usr/bin/env python3
"""Prepare a conservative Urdu candidate from a local WikiPron TSV.

WikiPron transcriptions are Wiktionary-derived data with their own licensing
terms. The generated file is an evaluation candidate, not a reviewed lexicon.
"""
import argparse
import collections
import csv
import hashlib
import io
import json
import pathlib
import sys

from build_lexicon import ROOT, build, normalize
from evaluate_lexicon import phonemes
from import_zuhri import eligible_word, units


def convert_phonemes(text):
    return units(text)


def convert(raw, confirmed=None):
    candidates = collections.defaultdict(dict)
    unsupported = set()
    rejected = collections.Counter()
    rows = csv.reader(io.StringIO(raw.decode("utf-8-sig"), newline=""), delimiter="\t")
    for number, row in enumerate(rows, 1):
        if len(row) != 2:
            raise ValueError(f"line {number}: expected word and phoneme sequence")
        word = normalize(row[0])
        if not eligible_word(word):
            rejected["unsupported word"] += 1
            continue
        try:
            reading = convert_phonemes(row[1])
        except ValueError as error:
            unsupported.add(word)
            rejected[f"unsupported {error}"] += 1
            continue
        encoded = json.dumps(reading, ensure_ascii=False, separators=(",", ":"))
        candidates[word][encoded] = reading
    entries = []
    for word, readings in sorted(candidates.items()):
        if word in unsupported:
            rejected["unsupported spelling"] += 1
        elif len(readings) != 1:
            rejected["conflicting spellings"] += 1
        else:
            reading = next(iter(readings.values()))
            if confirmed is not None and (word not in confirmed or len(confirmed[word]) != 1
                                          or phonemes(reading) != phonemes(confirmed[word][0])):
                rejected["not confirmed"] += 1
            else:
                entries.append({"word": word, "readings": [reading]})
    digest = hashlib.sha256(raw).hexdigest()
    return {"format": 1, "version": "wikipron-candidate-1",
            "provenance": "Evaluation candidate derived from WikiPron Urdu TSV, SHA-256 " + digest
                          + "; Wiktionary terms and token conversions require review",
            "entries": entries}, rejected


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("tsv", type=pathlib.Path)
    parser.add_argument("output", type=pathlib.Path)
    parser.add_argument("--confirm-with", type=pathlib.Path,
                        help="only retain readings also present in a validated JSON candidate")
    args = parser.parse_args()
    confirmed = None
    confirmation_hash = None
    if args.confirm_with:
        tables = json.loads((ROOT / "scripts/script_data.json").read_text(encoding="utf-8"))
        confirmation_bytes = args.confirm_with.read_bytes()
        confirmation_hash = hashlib.sha256(confirmation_bytes).hexdigest()
        source = build(json.loads(confirmation_bytes), tables)
        confirmed = {entry["word"]: entry["readings"] for entry in source["entries"]}
    result, rejected = convert(args.tsv.read_bytes(), confirmed)
    if confirmation_hash:
        result["provenance"] += "; confirmation JSON SHA-256 " + confirmation_hash
    args.output.write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(f"{len(result['entries'])} candidate spellings", file=sys.stderr)
    for reason, count in sorted(rejected.items()):
        print(f"{reason}: {count}", file=sys.stderr)


if __name__ == "__main__":
    main()
