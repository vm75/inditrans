#!/usr/bin/env python3
"""Convert a local Zuhri CSV to candidate canonical units for further review.

Only explicitly mapped phonemes are accepted. Conflicting readings for a
normalized spelling are omitted. This does not certify upstream data quality.
"""
import argparse
import collections
import csv
import hashlib
import io
import json
import pathlib
import sys
import unicodedata

from build_lexicon import normalize

# These are approximations in the existing Inditrans inventory, not IPA.
CONSONANTS = {
    "k": 0, "ɡ": 2, "g": 2, "ŋ": 4, "t͡ʃ": 5, "d͡ʒ": 7,
    "ʈ": 10, "ɖ": 12, "ɳ": 14, "t̪": 15, "t": 15, "d̪": 17,
    "d": 17, "n": 19, "p": 20, "b": 22, "m": 24, "j": 25,
    "r": 26, "ɾ": 26, "l": 27, "w": 28, "ʋ": 28, "ʃ": 29,
    "s": 31, "h": 32, "ɦ": 32, "q": 37, "x": 38, "ɣ": 39,
    "z": 40, "ɽ": 41, "f": 43,
}
VOWELS = {"ə": 0, "a": 0, "ɑ": 1, "aː": 1, "ɑː": 1,
          "ɪ": 2, "i": 2, "iː": 3, "ʊ": 4, "u": 4,
          "uː": 5, "eː": 11, "oː": 14}
ASPIRATED = {0: 1, 2: 3, 5: 6, 7: 8, 10: 11, 12: 13,
              15: 16, 17: 18, 20: 21, 22: 23, 41: 42}


def eligible_word(word):
    # Match the resolver's deliberately conservative, unmarked Arabic letters.
    for char in word:
        cp = ord(char)
        if (0x621 <= cp <= 0x626 or 0x674 <= cp <= 0x677) or cp in (0x6c0, 0x6c2, 0x6d3):
            return False
        if not (0x627 <= cp <= 0x63a or 0x641 <= cp <= 0x64a
                or 0x66e <= cp <= 0x66f or 0x671 <= cp <= 0x6d3
                or cp == 0x6d5 or 0x6ee <= cp <= 0x6ef
                or 0x6fa <= cp <= 0x6fc or 0x750 <= cp <= 0x77f):
            return False
    return bool(word)


def units(phonemes):
    result = []
    expanded = []
    for phoneme in phonemes.split():
        if phoneme[-1:] in ("ʰ", "ʱ") and phoneme[:-1] in CONSONANTS:
            expanded.extend((phoneme[:-1], phoneme[-1]))
        else:
            expanded.append(phoneme)
    for original in expanded:
        decomposed = unicodedata.normalize("NFD", original)
        nasal = decomposed.count("\u0303") == 1
        phoneme = decomposed.replace("\u0303", "") if nasal else decomposed
        if nasal and phoneme not in VOWELS:
            raise ValueError(original)
        if phoneme in CONSONANTS:
            result.append({"c": CONSONANTS[phoneme], "m": 0})
        elif phoneme in VOWELS:
            vowel = VOWELS[phoneme]
            if result and "c" in result[-1] and result[-1].get("m") == 0:
                if vowel == 0:
                    del result[-1]["m"]
                else:
                    result[-1]["m"] = vowel
            else:
                result.append({"v": vowel})
            if nasal:
                result[-1]["o"] = 0
        elif phoneme in ("ʰ", "ʱ") and result and "c" in result[-1] and result[-1].get("m") == 0:
            idx = result[-1]["c"]
            if (idx not in ASPIRATED or (phoneme == "ʰ" and idx not in (0, 5, 10, 15, 20))
                    or (phoneme == "ʱ" and idx not in (2, 7, 12, 17, 22, 41))):
                raise ValueError(phoneme)
            result[-1]["c"] = ASPIRATED[idx]
        else:
            raise ValueError(original)
    if not result:
        raise ValueError("empty")
    return result


def convert(raw):
    rows = csv.DictReader(io.StringIO(raw.decode("utf-8-sig"), newline=""))
    if not {"urdu_text", "phonemes", "is_valid"}.issubset(rows.fieldnames or ()):
        raise ValueError("expected Zuhri CSV columns")
    candidates = collections.defaultdict(dict)
    unsupported = set()
    rejected = collections.Counter()
    for row in rows:
        word = normalize(row["urdu_text"])
        if row["is_valid"].lower() != "true" or not eligible_word(word):
            rejected["invalid"] += 1
            continue
        try:
            reading = units(row["phonemes"])
        except ValueError as error:
            rejected[f"unsupported {error}"] += 1
            unsupported.add(word)
            continue
        encoded = json.dumps(reading, ensure_ascii=False, separators=(",", ":"))
        candidates[word][encoded] = reading
    entries = []
    for word, readings in sorted(candidates.items()):
        if word in unsupported:
            rejected["unsupported spelling"] += 1
            continue
        if len(readings) != 1:
            rejected["conflicting spellings"] += 1
            continue
        entries.append({"word": word, "readings": [next(iter(readings.values()))]})
    digest = hashlib.sha256(raw).hexdigest()
    return {"format": 1, "version": "zuhri-candidate-1",
            "provenance": "Candidate conversion of Zuhri; source SHA-256 " + digest
                          + "; license and data quality require review",
            "entries": entries}, rejected


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("csv", type=pathlib.Path)
    parser.add_argument("output", type=pathlib.Path)
    args = parser.parse_args()
    result, rejected = convert(args.csv.read_bytes())
    args.output.write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(f"{len(result['entries'])} candidate spellings", file=sys.stderr)
    for reason, count in sorted(rejected.items()):
        print(f"{reason}: {count}", file=sys.stderr)


if __name__ == "__main__":
    main()
