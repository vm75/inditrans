#!/usr/bin/env python3
"""Compare literal and lexicon-assisted Urdu output against token references.

The candidate and reference JSON are independently encoded in memory. The
engine's existing writer renders each reference, so this measures end-to-end
output on isolated words rather than comparing a Roman intermediary.
"""
import argparse
import ctypes
import hashlib
import json
import pathlib

from build_lexicon import ROOT, binary_pack, build
from import_zuhri import eligible_word


def pack(source):
    source = dict(source)
    source["source_sha256"] = hashlib.sha256(
        json.dumps(source, ensure_ascii=False, sort_keys=True).encode("utf-8")
    ).hexdigest()
    return binary_pack(source)


class Engine:
    def __init__(self, library):
        self.lib = ctypes.CDLL(str(library))
        self.lib.transliterate.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p,
                                         ctypes.c_ulong, ctypes.c_char_p, ctypes.c_char_p]
        self.lib.transliterate.restype = ctypes.c_void_p
        self.lib.transliterateUrduWithLexiconData.argtypes = [
            ctypes.c_char_p, ctypes.c_char_p, ctypes.c_ulong, ctypes.c_void_p,
            ctypes.c_ulong, ctypes.c_char_p, ctypes.c_char_p]
        self.lib.transliterateUrduWithLexiconData.restype = ctypes.c_void_p
        self.lib.releaseBuffer.argtypes = [ctypes.c_void_p]
        self.lib.releaseBuffer.restype = None

    def _read(self, result):
        if not result:
            raise ValueError("engine rejected input or target")
        try:
            return ctypes.string_at(result).decode("utf-8")
        finally:
            self.lib.releaseBuffer(result)

    def literal(self, word, target):
        return self._read(self.lib.transliterate(word.encode(), b"urdu", target.encode(), 0, b"##", b"##"))

    def assisted(self, word, target, data):
        buffer = ctypes.create_string_buffer(data)
        return self._read(self.lib.transliterateUrduWithLexiconData(
            word.encode(), target.encode(), 0, buffer, len(data), b"##", b"##"))


def evaluate(candidate, reference, target, engine):
    if target.lower() in ("urdu", "nastaliq"):
        raise ValueError("same-script Urdu output is not a pronunciation evaluation")
    candidate_words = {entry["word"] for entry in candidate["entries"]}
    data = pack(candidate)
    report = {"total": 0, "eligible": 0, "covered": 0, "baseline_exact": 0,
              "assisted_exact": 0, "corrected": 0, "false_corrections": 0,
              "unrenderable": 0, "errors": []}
    for entry in reference["entries"]:
        report["total"] += 1
        word = entry["word"]
        if not eligible_word(word):
            report["unrenderable"] += 1
            continue
        report["eligible"] += 1
        if word in candidate_words:
            report["covered"] += 1
        expected = []
        for reading in entry["readings"]:
            one = {"format": 1, "version": reference["version"], "provenance": reference["provenance"],
                   "entries": [{"word": word, "readings": [reading]}]}
            expected.append(engine.assisted(word, target, pack(one)))
        baseline = engine.literal(word, target)
        assisted = engine.assisted(word, target, data)
        was_correct = baseline in expected
        is_correct = assisted in expected
        report["baseline_exact"] += was_correct
        report["assisted_exact"] += is_correct
        report["corrected"] += not was_correct and is_correct
        report["false_corrections"] += was_correct and not is_correct
        if not is_correct:
            report["errors"].append({"word": word, "literal": baseline, "assisted": assisted, "expected": expected})
    eligible = report["eligible"]
    report["coverage"] = report["covered"] / eligible if eligible else None
    report["baseline_accuracy"] = report["baseline_exact"] / eligible if eligible else None
    report["assisted_accuracy"] = report["assisted_exact"] / eligible if eligible else None
    report["false_correction_rate"] = (report["false_corrections"] / report["baseline_exact"]
                                       if report["baseline_exact"] else None)
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("library", type=pathlib.Path, help="path to built native shared library")
    parser.add_argument("candidate", type=pathlib.Path)
    parser.add_argument("reference", type=pathlib.Path)
    parser.add_argument("--target", default="iast")
    args = parser.parse_args()
    tables = json.loads((ROOT / "scripts/script_data.json").read_text(encoding="utf-8"))
    candidate = build(json.loads(args.candidate.read_text(encoding="utf-8")), tables)
    reference = build(json.loads(args.reference.read_text(encoding="utf-8")), tables)
    print(json.dumps(evaluate(candidate, reference, args.target, Engine(args.library.resolve())),
                     ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
