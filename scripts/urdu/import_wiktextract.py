#!/usr/bin/env python3
"""Prepare a conservative Urdu candidate from a local Wiktextract JSONL file.

This does not certify the extract's provenance or the IPA-to-token mapping.
Multiple distinct readings, unsupported phonemes and unsupported words abstain.
"""
import argparse
import collections
import hashlib
import json
import pathlib
import sys
import unicodedata

from build_lexicon import normalize
from import_zuhri import CONSONANTS, VOWELS, eligible_word, units

NASAL_VOWELS = [vowel.removesuffix("ː") + "\u0303" + ("ː" if vowel.endswith("ː") else "")
                for vowel in VOWELS]
PHONEMES = sorted((*CONSONANTS, *VOWELS, *NASAL_VOWELS, "ʰ", "ʱ"), key=lambda s: (-len(s), s))
IGNORED = frozenset(".ˈˌ ")


def convert_ipa(ipa):
    # Slashes and brackets delimit a whole transcription, not alternatives.
    # A slash or bracket inside the transcription must not be silently joined.
    if len(ipa) < 2 or (ipa[0], ipa[-1]) not in (("/", "/"), ("[", "]")):
        raise ValueError("invalid IPA delimiters")
    ipa = unicodedata.normalize("NFD", ipa[1:-1])
    tokens = []
    pos = 0
    while pos < len(ipa):
        if ipa[pos] in IGNORED:
            pos += 1
            continue
        match = next((symbol for symbol in PHONEMES if ipa.startswith(symbol, pos)), None)
        if match is None:
            raise ValueError(ipa[pos])
        tokens.append(match)
        pos += len(match)
    return units(" ".join(tokens))


def convert(raw):
    candidates = collections.defaultdict(dict)
    unsupported = set()
    rejected = collections.Counter()
    for line in raw.decode("utf-8").splitlines():
        row = json.loads(line)
        if row.get("lang_code") != "ur":
            continue
        word = normalize(row.get("word", ""))
        if not eligible_word(word):
            rejected["unsupported word"] += 1
            continue
        sounds = [sound for sound in row.get("sounds", []) if sound.get("ipa")]
        if not sounds:
            # A second sense without a transcription may be a homograph; do
            # not silently select the pronunciation of another sense.
            unsupported.add(word)
            rejected["missing pronunciation"] += 1
            continue
        for sound in sounds:
            ipa = sound["ipa"]
            # Abstain rather than silently choose an unlabelled regional reading.
            tags = {tag.lower() for tag in sound.get("tags", [])}
            if sound.get("note") or tags - {"standard", "urdu", "phonemic"}:
                unsupported.add(word)
                rejected["nonstandard pronunciation"] += 1
                continue
            try:
                reading = convert_ipa(ipa)
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
            entries.append({"word": word, "readings": [next(iter(readings.values()))]})
    digest = hashlib.sha256(raw).hexdigest()
    return {"format": 1, "version": "wiktextract-candidate-1",
            "provenance": "Evaluation candidate derived from Wiktextract Urdu JSONL, SHA-256 " + digest
                          + "; per-file rights and token conversions require review",
            "entries": entries}, rejected


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("jsonl", type=pathlib.Path)
    parser.add_argument("output", type=pathlib.Path)
    args = parser.parse_args()
    result, rejected = convert(args.jsonl.read_bytes())
    args.output.write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(f"{len(result['entries'])} candidate spellings", file=sys.stderr)
    for reason, count in sorted(rejected.items()):
        print(f"{reason}: {count}", file=sys.stderr)


if __name__ == "__main__":
    main()
