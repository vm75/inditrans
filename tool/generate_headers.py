#!/usr/bin/env python3
"""Regenerate native, Dart, TypeScript, and JavaScript script/option tables.

Run from the repository root after editing the canonical JSON data. The
generated C++ reader entries are sorted by UTF-8 bytes because trie construction
depends on byte order, while public enum values retain the established name
ordering.
"""

from pathlib import Path
import sys

from python.codegen_data import LatinEquivalents, OptionHeaders, ScriptData, ScriptsHeaders
from python.static_scripts import StaticScripts


ROOT_SENTINEL = Path("flutter/pubspec.yaml")


def main() -> int:
    if not ROOT_SENTINEL.is_file():
        print("Run from root folder", file=sys.stderr)
        return 1

    scripts = ScriptData("tool/script_data.json", LatinEquivalents("docs/extended-latin.txt"))
    scripts.prepare_equivalents()
    StaticScripts(scripts.script_info_list, "tool/reader_data.json").write_header(
        "native/src/script_data.h"
    )

    ScriptsHeaders(scripts.script_list).update_dart("flutter/lib/src/script.dart")
    ScriptsHeaders(scripts.script_list).update_typescript("nodejs/src/Script.ts")
    ScriptsHeaders(scripts.script_list).update_javascript("js/src/inditrans.post.js")

    options = OptionHeaders("tool/options.json")
    options.update_native("native/src/exports.h")
    options.update_dart("flutter/lib/src/option.dart")
    options.update_typescript("nodejs/src/Option.ts")
    options.update_javascript("js/src/inditrans.post.js")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
