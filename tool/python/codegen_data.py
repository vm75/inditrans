"""Shared readers and renderers used by the source-code generators.

Generated output is checked in, so ordering and delimiters here are part of
the build contract. Keep renderers deterministic and fail clearly if a target
file no longer contains the expected insertion markers.
"""

from __future__ import annotations

from dataclasses import dataclass, field
import json
from pathlib import Path
import re


@dataclass
class ScriptInfo:
    category: str
    name: str
    info: dict

    @property
    def script_type(self) -> str:
        return self.category


class LatinEquivalents:
    """Read UnicodeData-style rows and map extended Latin code points to folds."""

    def __init__(self, path: str | Path, include_modifiers: bool = False):
        combining: dict[str, str] = {}
        modifiers: dict[str, str] = {}
        letters: dict[str, tuple[str, list[str], str | None]] = {}
        combining_re = re.compile(r"^.*?\t(.*?)\tCombining (.*)")
        modifier_re = re.compile(r"^.*?\t(.*?)\tModifier Letter (.*)")
        letter_re = re.compile(
            r"^.*?\t(.*?)\tLatin (Small|Capital) Letter ([A-Za-z])"
            r"(?: with (.*?))?(?: and (.*?))?(?: \(=(.*)\))?$"
        )

        for line in Path(path).read_text(encoding="utf-8").splitlines():
            if match := combining_re.match(line):
                combining[match.group(2).lower()] = match.group(1)
            elif include_modifiers and (match := modifier_re.match(line)):
                modifiers[match.group(2).lower()] = match.group(1)
            elif match := letter_re.match(line):
                names = [x.lower() for x in match.group(4, 5) if x is not None]
                letters[match.group(1)] = (
                    match.group(3).lower(), names, match.group(6)
                )

        self.equivalents: dict[int, list[str]] = {}
        for codepoint, (letter, names, alternate) in letters.items():
            if not names:
                continue
            d1 = combining.get(names[0], "")
            d2 = combining.get(names[1], "") if len(names) == 2 else ""
            m1 = modifiers.get(names[0], "") if include_modifiers else ""
            m2 = modifiers.get(names[1], "") if include_modifiers and len(names) == 2 else ""
            if not d1 and not d2:
                continue
            variants = [letter + d1 + d2]
            if include_modifiers and (m1 or m2):
                variants.append(letter + m1 + m2)
            if alternate is not None:
                variants.append(letter + d2 + d1)
                if include_modifiers and (m1 or m2):
                    variants.append(letter + m2 + m1)
            # The source generator indexes by UTF-16 code unit; these source
            # rows are all BMP, so the Unicode scalar value is the same index.
            self.equivalents[ord(codepoint)] = variants


ARRAY_TYPES = {
    "aliases": None,
    "vowels": 19,
    "vowelMarks": 19,
    "consonants": 50,
    "otherDiacritics": 4,
    "symbols": 13,
    "vedicSymbols": 3,
}

# The former Dart generator used its bundled Unicode casing table. Python may
# know newer uppercase forms for a few IPA letters, which would silently add
# different reader spellings. Keep these mappings frozen until script data
# deliberately adopts the newer forms.
CASE_STABLE_UPPER = {0x0261, 0x0266, 0x026A, 0x0282}


def legacy_upper(text: str) -> str:
    return "".join(char if ord(char) in CASE_STABLE_UPPER else char.upper() for char in text)


class ScriptData:
    """Load script definitions, add derived Latin spellings, and retain source order."""

    def __init__(self, path: str | Path, latin_equivalents: LatinEquivalents):
        data = json.loads(Path(path).read_text(encoding="utf-8"))
        self.script_info_list = [
            ScriptInfo(category, name, info)
            for category, scripts in data.items()
            for name, info in scripts.items()
        ]
        self.script_info_list.sort(key=lambda script: script.name)
        self.script_list = []
        for script in self.script_info_list:
            self.script_list.append(script.name)
            self.script_list.extend(script.info.get("aliases", []))
        self.script_list.append("indic")
        self.latin_equivalents = latin_equivalents

    @staticmethod
    def _utf16_units(text: str) -> list[int]:
        encoded = text.encode("utf-16-le", errors="surrogatepass")
        return [encoded[i] | encoded[i + 1] << 8 for i in range(0, len(encoded), 2)]

    @staticmethod
    def _from_utf16_unit(unit: int) -> str:
        return bytes((unit & 255, unit >> 8)).decode("utf-16-le", errors="surrogatepass")

    def _add_equivalent(self, spelling: str, equivalents: dict, uppercase: bool) -> None:
        values = equivalents.get(spelling, [])
        changed = False
        if uppercase:
            upper = legacy_upper(spelling)
            if upper != spelling and upper not in values:
                values.append(upper)
                changed = True

        parts = []
        for unit in self._utf16_units(spelling):
            options = self.latin_equivalents.equivalents.get(unit)
            if options:
                parts.append(options[0])
                changed = True
            else:
                parts.append(self._from_utf16_unit(unit))
        folded = "".join(parts)
        if changed and folded != spelling and folded not in values:
            values.append(folded)
            if uppercase:
                upper_folded = legacy_upper(folded)
                if upper_folded != folded and upper_folded not in values:
                    values.append(upper_folded)
        if changed:
            equivalents[spelling] = values

    def prepare_equivalents(self) -> None:
        """Add uppercase/diacritic-free inputs expected by the Latin readers."""
        for script in self.script_info_list:
            if script.category != "latin":
                continue
            equivalents = script.info.setdefault("equivalents", {})
            uppercase = script.name in {"iast", "ipa", "iso"}
            for category in ARRAY_TYPES:
                for spelling in script.info.get(category, []):
                    self._add_equivalent(spelling, equivalents, uppercase)


def replace_by_delimiters(path: str | Path, start: str, end: str, replacement: str) -> None:
    """Replace text between stable markers while keeping the markers intact."""
    path = Path(path)
    contents = path.read_text(encoding="utf-8")
    start_marker = start if start.endswith("\n") else start + "\n"
    start_index = contents.find(start_marker)
    end_index = contents.find(end, start_index + len(start_marker)) if start_index >= 0 else -1
    if start_index < 0 or end_index < 0:
        raise ValueError(f"Could not find generator markers in {path}: {start!r}, {end!r}")
    path.write_text(
        contents[:start_index + len(start_marker)] + replacement + contents[end_index:],
        encoding="utf-8",
    )


class ScriptsHeaders:
    """Render the supported-script enum and update its generated counterparts."""

    def __init__(self, scripts: list[str]):
        self.scripts = sorted(scripts)

    def update_dart(self, path: str) -> None:
        replace_by_delimiters(path, "enum Script {", "}", "".join(f"  {s},\n" for s in self.scripts))

    def update_typescript(self, path: str) -> None:
        lines = ["// GENERATED CODE - DO NOT MODIFY BY HAND", "", "/// Supported scripts", "export enum Script {"]
        lines += [f"  {script} = '{script}'," for script in self.scripts]
        lines.append("}")
        Path(path).write_text("\n".join(lines) + "\n", encoding="utf-8")

    def update_javascript(self, path: str) -> None:
        replacement = "".join(f"            '{script}',\n" for script in self.scripts)
        replace_by_delimiters(path, "'Scripts': [\n", "        ],", replacement)


@dataclass
class OptionInfo:
    name: str
    comment: str
    value: int


class OptionHeaders:
    """Render option declarations for the native, Dart, TypeScript, and JS APIs."""

    def __init__(self, path: str):
        self.options = [OptionInfo(row["name"], row["comment"], row["value"])
                        for row in json.loads(Path(path).read_text(encoding="utf-8"))]

    def update_dart(self, path: str) -> None:
        lines = ["  /// A map of option names to their integer values", "  static final _valueMap = <String, int>{"]
        lines += [f"    '{o.name.lower()}': {o.value}," for o in self.options]
        lines += ["  };", ""]
        for option in self.options:
            lines += [f"  /// {option.comment}", f"  static final {option.name} = Option._({option.value});", ""]
        replace_by_delimiters(path, "class Option {", "  /// Returns the int value of the option flag", "\n".join(lines) + "\n")

    def update_typescript(self, path: str) -> None:
        lines = ["// GENERATED CODE - DO NOT MODIFY BY HAND", "", "/// Transliteration options", "/// Flags to control transliteration", "export enum Option {"]
        for index, option in enumerate(self.options):
            if index:
                lines.append("")
            lines += [f"  /// {option.comment}", f"  {option.name} = {option.value},"]
        lines += ["}"]
        Path(path).write_text("\n".join(lines) + "\n", encoding="utf-8")

    def update_native(self, path: str) -> None:
        path = Path(path)
        contents = path.read_text(encoding="utf-8")
        marker = "enum TranslitOptions {"
        begin = contents.find(marker)
        end = contents.find("};", begin)
        if begin < 0 or end < 0:
            raise ValueError(f"Could not find TranslitOptions enum in {path}")
        lines = [marker]
        for option in self.options:
            lines += [f"  /// {option.comment}", f"  {option.name} = {option.value},"]
        path.write_text(contents[:begin] + "\n".join(lines) + "\n" + contents[end:], encoding="utf-8")

    def update_javascript(self, path: str) -> None:
        replacement = "".join(f"            '{o.name}': {o.value},\n" for o in self.options)
        replace_by_delimiters(path, "'Options': {\n", "        },", replacement)
