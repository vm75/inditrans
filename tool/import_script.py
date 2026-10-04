#!/usr/bin/env python3
"""Import script character maps from the upstream Aksharamukha data set."""

import json
from pathlib import Path
import sys
from urllib.request import urlopen


URL = ("https://raw.githubusercontent.com/virtualvinodh/aksharamukha/master/"
       "aksharamukha-back/resources/script_mapping/script_mapping.json")
CATEGORIES = ("vedic", "indic", "latin", "tamil")
METADATA = {"description", "nonSupportingPrograms", "comments", "TODO", "dev"}


def value_at(source: dict, dotted_path: str):
    """Follow a dotted key path in the nested upstream map."""
    value = source
    for part in dotted_path.split("."):
        value = value[part]
    return value


def make_script(name: str, source: dict) -> dict:
    """Select the shared engine's fixed character classes from upstream fields."""
    def take(path: str, start: int, count: int = 1) -> list[str]:
        return list(value_at(source, path)[start:start + count])

    vowels = (take("vowels.main", 0, 10) + take("vowels.south", 0)
              + take("vowels.main", 10, 2) + take("vowels.south", 1)
              + take("vowels.main", 12, 2) + take("vowels.modern", 0)
              + take("vowels.sinhala", 0) + take("vowels.modern", 1))
    vowel_marks = [value_at(source, "vowelsigns.virama")[0]]
    if vowel_marks[0] == "×":
        vowel_marks[0] = ""
    vowel_marks += (take("vowelsigns.main", 0, 9) + take("vowelsigns.south", 0)
                    + take("vowelsigns.main", 9, 2) + take("vowelsigns.south", 1)
                    + take("vowelsigns.main", 11, 2) + take("vowelsigns.modern", 0)
                    + take("vowelsigns.sinhala", 0) + take("vowelsigns.modern", 1))
    vedic_symbols = take("others.symbols", 0)
    vedic_symbols += ["gͫ", "gͫ̄"] if vowel_marks[0] == "" else ["ꣳ", "ꣴ"]
    return {
        "vowels": vowels,
        "vowelMarks": vowel_marks,
        "consonants": (take("consonants.main", 0, 33) + take("consonants.south", 0, 4)
                       + take("consonants.persoarabic", 0, 8) + take("consonants.sinhala", 0, 5)),
        "otherDiacritics": take("combiningsigns.ayogavaha", 0, 3) + take("others.aytham", 0),
        "symbols": take("numerals", 0, 10) + take("others.om", 0) + take("others.symbols", 1, 2),
        "vedicSymbols": vedic_symbols,
        "equivalents": {"x:0": ["()", "^"], "x:1": ["{}", "^^"]},
    }


def compact_json(value) -> str:
    """Match the legacy layout: objects are indented, arrays stay on one line."""
    if isinstance(value, dict):
        if not value:
            return "{}"
        rows = []
        for key, item in value.items():
            rows.append(f'  "{key}": {compact_json(item)}')
        # Child indentation is added recursively to object lines only.
        return "{\n" + ",\n".join(rows) + "\n}"
    if isinstance(value, list):
        if not value:
            return "[]"
        return "[ " + ", ".join(json.dumps(x, ensure_ascii=False) for x in value) + " ]"
    return json.dumps(value, ensure_ascii=False)


def format_object(value: dict, depth: int = 0) -> str:
    """Indent nested objects while keeping every JSON array on one line."""
    pad = "  " * depth
    if not value:
        return "{}"
    rows = []
    for key, item in value.items():
        rendered = format_object(item, depth + 1) if isinstance(item, dict) else compact_json(item)
        rows.append(f'{"  " * (depth + 1)}"{key}": {rendered}')
    return "{\n" + ",\n".join(rows) + f"\n{pad}}}"


def main(argv: list[str]) -> int:
    if len(argv) < 2:
        print(f"Usage: python3 tool/import_script.py <category> <script1> [script2] ...\nCategories: {', '.join(CATEGORIES)}")
        return 2
    category = argv[0].lower()
    if category not in CATEGORIES:
        print(f"Invalid category: {category}\nValid categories: {', '.join(CATEGORIES)}", file=sys.stderr)
        return 2
    path = Path("tool/script_data.json")
    if not Path("flutter/pubspec.yaml").is_file():
        print("Run from repository root", file=sys.stderr)
        return 1
    if path.exists():
        script_data = json.loads(path.read_text(encoding="utf-8"))
        print("Loaded existing script_data.json")
    else:
        script_data = {name: {} for name in CATEGORIES}
        print("Creating new script_data.json")

    print("Fetching script mapping from remote...")
    with urlopen(URL, timeout=30) as response:
        upstream = json.load(response)
    added = 0
    for name in argv[1:]:
        if name not in upstream:
            print(f'❌ Script "{name}" not found in remote mapping')
            continue
        if any(isinstance(entries, dict) and name in entries for entries in script_data.values()):
            print(f'⏭️  Script "{name}" already exists, skipping')
            continue
        try:
            script_data[category][name] = make_script(name, upstream[name])
            print(f'✅ Added "{name}" to category "{category}"')
            added += 1
        except (KeyError, IndexError, TypeError) as error:
            print(f'❌ Error importing "{name}": {error}')

    if added:
        path.write_text(format_object(script_data) + "\n", encoding="utf-8")
        print(f'\n✨ Successfully saved script_data.json\n   Added: {added} scripts to "{category}"')
    else:
        print("\n⚠️  No new scripts were added")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
