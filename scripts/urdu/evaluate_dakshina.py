#!/usr/bin/env python3
"""Compare Urdu output with human Roman spellings from a local Dakshina TSV.

This is a conservative string-agreement screen, not phoneme accuracy: Dakshina
romanizations are not a standardized pronunciation transcription. Input data
remain local and are not redistributed by this tool.
"""
import argparse
import collections
import csv
import hashlib
import io
import json
import pathlib
import re
import unicodedata

from build_lexicon import ROOT, build, normalize
from evaluate_runtime import Engine, pack
from import_zuhri import eligible_word


def fold_latin(text):
    # Approximate only case, diacritics and repeated Latin vowels. In
    # particular do not invent equivalences such as English "ee" -> "i".
    letters = "".join(char for char in unicodedata.normalize("NFKD", text.casefold())
                      if "a" <= char <= "z")
    return re.sub(r"([aeiou])\1+", r"\1", letters)


def references(raw):
    words = collections.defaultdict(set)
    for number, row in enumerate(csv.reader(io.StringIO(raw.decode("utf-8-sig"), newline=""), delimiter="\t"), 1):
        if len(row) != 3 or not row[0] or not row[1] or not row[2].isdigit() or int(row[2]) <= 0:
            raise ValueError(f"line {number}: expected word, Roman spelling and positive annotation count")
        word = normalize(row[0])
        spelling = fold_latin(row[1])
        if spelling:
            words[word].add(spelling)
    return words


def evaluate(candidate, gold, engine):
    candidate_words = {entry["word"] for entry in candidate["entries"]}
    data = pack(candidate)
    report = {"words": len(gold), "eligible": 0, "covered": 0, "covered_multiple_spellings": 0,
              "baseline_agrees": 0, "assisted_agrees": 0, "gained_agreement": 0, "lost_agreement": 0,
              "covered_baseline_agrees": 0, "covered_assisted_agrees": 0}
    for word, spellings in sorted(gold.items()):
        if not eligible_word(word):
            continue
        report["eligible"] += 1
        covered = word in candidate_words
        if covered:
            report["covered"] += 1
            report["covered_multiple_spellings"] += len(spellings) > 1
        baseline = fold_latin(engine.literal(word, "iast")) in spellings
        assisted = fold_latin(engine.assisted(word, "iast", data)) in spellings
        report["baseline_agrees"] += baseline
        report["assisted_agrees"] += assisted
        report["gained_agreement"] += not baseline and assisted
        report["lost_agreement"] += baseline and not assisted
        report["covered_baseline_agrees"] += covered and baseline
        report["covered_assisted_agrees"] += covered and assisted
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("library", type=pathlib.Path)
    parser.add_argument("candidate", type=pathlib.Path)
    parser.add_argument("reference_tsv", type=pathlib.Path)
    args = parser.parse_args()
    tables = json.loads((ROOT / "scripts/script_data.json").read_text(encoding="utf-8"))
    candidate_bytes = args.candidate.read_bytes()
    candidate = build(json.loads(candidate_bytes), tables)
    reference_bytes = args.reference_tsv.read_bytes()
    report = evaluate(candidate, references(reference_bytes), Engine(args.library.resolve()))
    report["reference_sha256"] = hashlib.sha256(reference_bytes).hexdigest()
    report["candidate_sha256"] = hashlib.sha256(candidate_bytes).hexdigest()
    report["caveat"] = "Folded Roman spelling agreement is not phoneme accuracy or a false-correction rate."
    print(json.dumps(report, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
