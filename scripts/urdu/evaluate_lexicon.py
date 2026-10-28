#!/usr/bin/env python3
"""Evaluate an Urdu candidate against an independently prepared token reference.

Input files use build_lexicon.py's JSON source format. This measures the
lexicon's selected canonical phonemes for isolated words, not end-to-end
transliteration or sentence disambiguation.
"""
import argparse
import json
import pathlib

from build_lexicon import ROOT, build
from import_zuhri import eligible_word


def phonemes(reading):
    result = []
    for unit in reading:
        if "v" in unit:
            result.append(("v", unit["v"]))
        else:
            result.append(("c", unit["c"]))
            vowel = unit.get("m")
            if vowel is None:
                result.append(("v", 0))
            elif vowel != 0:
                result.append(("v", vowel))
        if "o" in unit:
            result.append(("o", unit["o"]))
    return result


def edit_distance(left, right):
    row = list(range(len(right) + 1))
    for i, a in enumerate(left, 1):
        next_row = [i]
        for j, b in enumerate(right, 1):
            next_row.append(min(next_row[-1] + 1, row[j] + 1, row[j - 1] + (a != b)))
        row = next_row
    return row[-1]


def evaluate(candidate, reference):
    lookup = {entry["word"]: entry["readings"] for entry in candidate["entries"]}
    report = {"total": 0, "covered": 0, "ambiguous_reference": 0, "ambiguous_covered": 0, "oov_or_unsupported": 0,
              "exact": 0, "phoneme_edits": 0, "reference_phonemes": 0, "errors": []}
    for entry in reference["entries"]:
        word = entry["word"]
        report["total"] += 1
        if len(entry["readings"]) > 1:
            report["ambiguous_reference"] += 1
        readings = lookup.get(word) if eligible_word(word) else None
        if not readings:
            report["oov_or_unsupported"] += 1
            continue
        report["covered"] += 1
        references = [phonemes(reading) for reading in entry["readings"]]
        predicted = phonemes(readings[0])
        if len(readings) > 1 or len(references) > 1:
            report["ambiguous_covered"] += 1
        if predicted in references:
            report["exact"] += 1
        else:
            report["errors"].append(word)
        # Choose the closest accepted reference; ties use the original order.
        best = min(references, key=lambda reference: edit_distance(predicted, reference))
        report["phoneme_edits"] += edit_distance(predicted, best)
        report["reference_phonemes"] += len(best)
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("candidate", type=pathlib.Path)
    parser.add_argument("reference", type=pathlib.Path)
    args = parser.parse_args()
    tables = json.loads((ROOT / "scripts/script_data.json").read_text(encoding="utf-8"))
    candidate = build(json.loads(args.candidate.read_text(encoding="utf-8")), tables)
    reference = build(json.loads(args.reference.read_text(encoding="utf-8")), tables)
    print(json.dumps(evaluate(candidate, reference), ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
