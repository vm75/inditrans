#!/usr/bin/env python3
"""Compare sentence outputs using explicitly annotated Urdu word readings.

Each sentence supplies one contextual reading per distinct eligible word.
Other text follows the legacy reader, so the results are not a substitute for
a fully annotated, human-reviewed sentence corpus.
"""
import argparse
import json
import pathlib

from build_lexicon import ROOT, build, normalize
from evaluate_runtime import Engine, pack
from import_zuhri import eligible_word


def boundary(char):
    code = ord(char)
    return (code <= 32 or 33 <= code <= 47 or 58 <= code <= 64
            or 91 <= code <= 96 or 123 <= code <= 126 or char in "،؛؟")


def scan_words(text):
    """Return eligible words and unsupported Urdu spans outside protected text."""
    result = []
    unsupported = []
    pos = 0
    while pos < len(text):
        if text.startswith("##", pos):
            end = text.find("##", pos + 2)
            if end < 0:
                break
            pos = end + 2
        elif text[pos] == "<":
            end = text.find(">", pos + 1)
            if end < 0:
                break
            pos = end + 1
        elif boundary(text[pos]):
            pos += 1
        else:
            end = pos + 1
            while end < len(text) and not boundary(text[end]):
                end += 1
            word = text[pos:end]
            if eligible_word(word):
                result.append(normalize(word))
            elif any(0x600 <= ord(char) <= 0x6ff or 0x750 <= ord(char) <= 0x77f
                     or 0xfb50 <= ord(char) <= 0xfdff or 0xfe70 <= ord(char) <= 0xfeff
                     for char in word):
                unsupported.append(word)
            pos = end
    return result, unsupported


def eligible_words(text):
    return scan_words(text)[0]


def evaluate(candidate, reference, target, engine, tables):
    if target.lower() in ("urdu", "nastaliq"):
        raise ValueError("same-script Urdu output is not a pronunciation evaluation")
    if reference.get("format") != 1 or not reference.get("version") or not reference.get("provenance"):
        raise ValueError("sentence reference format, version and provenance are required")
    candidate_words = {entry["word"] for entry in candidate["entries"]}
    candidate_pack = pack(candidate)
    report = {"sentences": 0, "scored_sentences": 0, "unsupported_sentences": 0,
              "eligible_words": 0, "covered_words": 0,
              "baseline_exact": 0, "assisted_exact": 0, "corrected": 0,
              "false_corrections": 0, "errors": []}
    for item in reference["sentences"]:
        text = item["text"]
        if not isinstance(text, str) or not text or "\0" in text:
            raise ValueError("sentence text must be nonempty and contain no NUL")
        words, unsupported = scan_words(text)
        source = {"format": 1, "version": reference["version"], "provenance": reference["provenance"],
                  "entries": item["readings"]}
        gold = build(source, tables)
        if any(len(entry["readings"]) != 1 for entry in gold["entries"]):
            raise ValueError("each contextual word must have exactly one reading")
        if set(words) != {entry["word"] for entry in gold["entries"]}:
            raise ValueError("reference must annotate exactly the eligible words outside protected spans")
        report["sentences"] += 1
        if unsupported or not words:
            # Literal fallback is not an independently annotated pronunciation.
            report["unsupported_sentences"] += 1
            continue
        expected = engine.assisted(text, target, pack(gold))
        baseline = engine.literal(text, target)
        assisted = engine.assisted(text, target, candidate_pack)
        was_correct = baseline == expected
        is_correct = assisted == expected
        report["scored_sentences"] += 1
        report["eligible_words"] += len(words)
        report["covered_words"] += sum(word in candidate_words for word in words)
        report["baseline_exact"] += was_correct
        report["assisted_exact"] += is_correct
        report["corrected"] += not was_correct and is_correct
        report["false_corrections"] += was_correct and not is_correct
        if not is_correct:
            report["errors"].append({"text": text, "literal": baseline, "assisted": assisted, "expected": expected})
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("library", type=pathlib.Path)
    parser.add_argument("candidate", type=pathlib.Path)
    parser.add_argument("reference", type=pathlib.Path)
    parser.add_argument("--target", default="iast")
    args = parser.parse_args()
    tables = json.loads((ROOT / "scripts/script_data.json").read_text(encoding="utf-8"))
    candidate = build(json.loads(args.candidate.read_text(encoding="utf-8")), tables)
    reference = json.loads(args.reference.read_text(encoding="utf-8"))
    result = evaluate(candidate, reference, args.target, Engine(args.library.resolve()), tables)
    print(json.dumps(result, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
