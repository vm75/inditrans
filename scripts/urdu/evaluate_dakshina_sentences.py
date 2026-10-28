#!/usr/bin/env python3
"""Screen sentence output against local human Romanized Dakshina text.

This is folded string agreement, not sentence phoneme accuracy. Sentences with
unsupported Urdu spans are tracked; non-Latin output is excluded from scoring.
"""
import argparse
import hashlib
import json
import pathlib
import re
import unicodedata

from build_lexicon import ROOT, build
from evaluate_runtime import Engine, pack
from evaluate_sentences import scan_words


def fold_sentence(text):
    words = []
    for word in text.split():
        chars = []
        for char in unicodedata.normalize("NFKD", word.casefold()):
            if "a" <= char <= "z":
                chars.append(char)
            elif char.isdecimal():
                chars.append(str(unicodedata.decimal(char)))
            elif unicodedata.category(char).startswith("L"):
                return None
        folded = re.sub(r"([aeiou])\1+", r"\1", "".join(chars))
        if folded:
            words.append(folded)
    return " ".join(words)


def references(raw):
    rows = []
    for number, line in enumerate(raw.decode("utf-8-sig").splitlines(), 1):
        parts = line.split("\t")
        if len(parts) != 2 or not all(parts) or "\0" in line:
            raise ValueError(f"line {number}: expected native and Roman sentence")
        rows.append((parts[0], parts[1]))
    return rows


def evaluate(candidate, rows, engine):
    words_in_pack = {entry["word"] for entry in candidate["entries"]}
    data = pack(candidate)
    report = {"sentences": len(rows), "unsupported_urdu": 0, "no_urdu_words": 0,
              "non_latin_output": 0, "scored": 0, "with_covered_word": 0,
              "scored_with_unsupported_urdu": 0,
              "baseline_agrees": 0, "assisted_agrees": 0, "gained_agreement": 0,
              "lost_agreement": 0, "unexpected_change_without_hit": 0}
    for text, roman in rows:
        words, unsupported = scan_words(text)
        if unsupported:
            report["unsupported_urdu"] += 1
        if not words and not unsupported:
            report["no_urdu_words"] += 1
            continue
        reference = fold_sentence(roman)
        baseline_raw = engine.literal(text, "iast")
        assisted_raw = engine.assisted(text, "iast", data)
        covered = any(word in words_in_pack for word in words)
        report["unexpected_change_without_hit"] += not covered and baseline_raw != assisted_raw
        baseline = fold_sentence(baseline_raw)
        assisted = fold_sentence(assisted_raw)
        if reference is None or baseline is None or assisted is None:
            report["non_latin_output"] += 1
            continue
        report["scored"] += 1
        report["scored_with_unsupported_urdu"] += bool(unsupported)
        report["with_covered_word"] += covered
        literal_match, assisted_match = baseline == reference, assisted == reference
        report["baseline_agrees"] += literal_match
        report["assisted_agrees"] += assisted_match
        report["gained_agreement"] += not literal_match and assisted_match
        report["lost_agreement"] += literal_match and not assisted_match
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
    result = evaluate(candidate, references(reference_bytes), Engine(args.library.resolve()))
    result["candidate_sha256"] = hashlib.sha256(candidate_bytes).hexdigest()
    result["reference_sha256"] = hashlib.sha256(reference_bytes).hexdigest()
    result["caveat"] = "Folded sentence spelling agreement is not phoneme accuracy or a false-correction rate."
    print(json.dumps(result, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
