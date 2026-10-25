#!/usr/bin/env python3
"""Download upstream common tests and reshape them into the shared JSON suite."""

import json
from pathlib import Path
import sys
from urllib.request import urlopen


URL = ("https://raw.githubusercontent.com/indic-transliteration/common_tests/"
       "master/transliterationTests.json")


def convert(source: dict) -> list[dict]:
    """Preserve upstream iteration order while expanding its four test groups."""
    tests: list[dict] = []
    basic_types: dict[str, dict] = {}
    for script, cases in source["basic_all_to_all"].items():
        for label, text in cases.items():
            description = f"any-to-any: {label}"
            case = basic_types.setdefault(description, {
                "description": description, "type": "any-to-any", "targets": []
            })
            case["targets"].append({"script": script, "text": text})
    tests.extend(basic_types.values())

    metadata = {"description", "nonSupportingPrograms", "comments", "TODO", "dev"}
    for entry in source["devanaagarii_round_trip"]:
        tests.append({
            "description": f"to-and-fro: {entry['description']}",
            "type": "to-and-fro", "script": "devanagari", "text": entry["dev"],
            "targets": [
                {"script": script, "text": value}
                for script, value in entry.items() if script not in metadata
            ],
        })

    for entry in source["to_devanaagarii"]:
        # The upstream format represents one target in these rows. Retain its
        # first script field so generated case order remains stable.
        target = next(((script, value) for script, value in entry.items() if script not in metadata), None)
        if target is not None:
            tests.append({
                "description": entry["description"], "script": target[0], "text": target[1],
                "targets": [{"script": "devanagari", "text": entry["dev"]}],
            })

    for entry in source["from_devanaagarii"]:
        tests.append({
            "description": entry["description"], "script": "devanagari", "text": entry["dev"],
            "targets": [
                {"script": script, "text": value}
                for script, value in entry.items() if script not in metadata
            ],
        })
    return tests


def main() -> int:
    if not Path("flutter/pubspec.yaml").is_file():
        print("Run from repository root", file=sys.stderr)
        return 1
    with urlopen(URL, timeout=30) as response:
        source = json.load(response)
    output = json.dumps(convert(source), ensure_ascii=False, indent=2)
    Path("test-files/sanscript-tests.json").write_text(output, encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
