#!/usr/bin/env python3
"""Compare old/new C libraries over script spellings, aliases, options and mixed text.

--spellings accepts legacy-generated TSV rows (script name, spelling). This
includes Latin alternatives synthesized by the old header generator. Optional
token probes export probe_lookup(source, text, uint32_t* tokens, uint32_t* length)
and pack type/index/script type at bit offsets 0/8/16.
"""

from __future__ import annotations

import argparse
import ctypes
import json
import random
from pathlib import Path


class Library:
    def __init__(self, path: Path):
        self.lib = ctypes.CDLL(str(path.resolve()), mode=ctypes.RTLD_LOCAL)
        self.lib.transliterate.argtypes = [ctypes.c_char_p] * 3 + [ctypes.c_ulong] + [ctypes.c_char_p] * 2
        self.lib.transliterate.restype = ctypes.c_void_p
        self.lib.releaseBuffer.argtypes = [ctypes.c_void_p]
        self.lib.isScriptSupported.argtypes = [ctypes.c_char_p]
        self.lib.isScriptSupported.restype = ctypes.c_int

    def translate(self, text: str, source: str, target: str, options: int = 0,
                  skip_start: str = "##", skip_end: str = "##"):
        pointer = self.lib.transliterate(text.encode(), source.encode(), target.encode(), options,
                                         skip_start.encode(), skip_end.encode())
        if not pointer:
            return None
        try:
            return ctypes.string_at(pointer)
        finally:
            self.lib.releaseBuffer(pointer)


class TokenProbe:
    def __init__(self, path: Path):
        self.lib = ctypes.CDLL(str(path.resolve()), mode=ctypes.RTLD_LOCAL)
        self.call = self.lib.probe_lookup
        self.call.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.POINTER(ctypes.c_uint), ctypes.POINTER(ctypes.c_uint)]
        self.call.restype = ctypes.c_int
        self.tokens = (ctypes.c_uint * 256)()
        self.length = ctypes.c_uint()

    def lookup(self, source: str, text: str):
        self.length.value = 0
        count = self.call(source.encode(), text.encode(), self.tokens, ctypes.byref(self.length))
        return count, self.length.value, tuple(self.tokens[:max(count, 0)])


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--before", type=Path, required=True)
    parser.add_argument("--after", type=Path, required=True)
    parser.add_argument("--spellings", type=Path)
    parser.add_argument("--tokens-before", type=Path)
    parser.add_argument("--tokens-after", type=Path)
    args = parser.parse_args()
    root = Path(__file__).resolve().parent.parent
    source_data = json.loads((root / "tool/script_data.json").read_text())
    scripts = {name: info for group in source_data.values() for name, info in group.items()}
    non_roman = {name for kind, group in source_data.items() if kind != "latin" for name in group}
    aliases = {name: name for name in scripts}
    for name, info in scripts.items():
        aliases.update({alias: name for alias in info.get("aliases", [])})
    spellings: dict[str, set[str]] = {name: set() for name in scripts}
    for name, info in scripts.items():
        for field in ("vowels", "vowelMarks", "consonants", "otherDiacritics", "symbols", "vedicSymbols"):
            spellings[name].update(info.get(field, []))
        for values in info.get("equivalents", {}).values():
            spellings[name].update(values)
    if args.spellings:
        for row in args.spellings.read_text().splitlines():
            name, key = row.split("\t", 1)
            spellings.setdefault(name, set()).add(key)
    spellings["indic"] = set().union(*(spellings[name] for name in non_roman), spellings.get("indic", set()))
    aliases["indic"] = "indic"
    all_keys = sorted(set().union(*spellings.values()) - {""})
    names = sorted(set(aliases) | {name.upper() for name in aliases} | {"unknown", "readablelatin"})
    before, after = Library(args.before), Library(args.after)
    checks = 0

    def compare(text: str, source: str, target: str, options: int = 0,
                skip_start: str = "##", skip_end: str = "##"):
        nonlocal checks
        old = before.translate(text, source, target, options, skip_start, skip_end)
        new = after.translate(text, source, target, options, skip_start, skip_end)
        checks += 1
        if old != new:
            raise AssertionError((source, target, options, repr(text), old, new))

    for name in names:
        assert before.lib.isScriptSupported(name.encode()) == after.lib.isScriptSupported(name.encode()), name
    if bool(args.tokens_before) != bool(args.tokens_after):
        parser.error("both token probes are required")
    if args.tokens_before:
        probes = TokenProbe(args.tokens_before), TokenProbe(args.tokens_after)
        token_checks = 0
        for name in names:
            for text in all_keys + [key + "!" for key in all_keys] + ["Kh", "kH", "🙂", "?"]:
                old, new = (probe.lookup(name, text) for probe in probes)
                assert old == new, (name, text, old, new)
                token_checks += 1
        print(f"Token comparisons passed: {token_checks}", flush=True)

    for name in names:
        canonical = aliases.get(name, aliases.get(name.lower()))
        keys = spellings.get(canonical, {"namaste", "க", "क़"})
        for text in sorted(keys | {""}):
            for target in scripts:
                compare(text, name, target)
    print(f"Spelling/alias/target comparisons passed: {checks}", flush=True)

    samples = [
        "Kh kH KH śrī gurubhyo namaḥ", "அது இது மா ஒரு அந்த இந்த கதி",
        "അവൻ അവൾ കൽ ൿ ൔ ൕ ൖ", "க² க₄ ஜ² ஃ ʼˮˇ", "ੱਕ ੴ",
        "क़ क़ ओ३म् अॅ ॲ", "நமஸ்தே नमस्ते నమస్తే নমস্তে नमঃ ।",
        "##raw नमस्ते🙂## भारतम्", "<tag नमस्ते>भारत</tag>", "<unterminated",
        "unknown text 🙂 123", "॒॑᳚‌‍​ꣽ",
    ]
    rng = random.Random(0)
    samples += [" ".join(rng.sample(all_keys, 8)) for _ in range(8)]
    for source in names:
        for target in scripts:
            for options in (0, 1, 2, 4, 8, 16, 32, 64, 127):
                for text in samples:
                    compare(text, source, target, options)
    for source, target in (("indic", "iso"), ("tamil", "devanagari"), ("iso", "tamil")):
        for options in range(128):
            for text in samples:
                compare(text, source, target, options)
    # Delimiters are byte strings; recognized glyphs and XML retain precedence.
    for source in ("indic", "devanagari", "iso"):
        for target in ("iso", "telugu", "tamil"):
            for options in (0, 1, 2, 4, 8, 16, 32, 64, 127):
                for start, end in (("##", "##"), ("[[", "]]"), ("<!--", "-->"), ("क", "ख"), ("##", "")):
                    for text in (start + "raw नमस्ते🙂" + end + " भारतम्", start + "unclosed नमस्ते",
                                 start + end + end, "नमस्ते <tag raw नमस्ते>" + start + "x" + end):
                        compare(text, source, target, options, start, end)
    print(f"All C API comparisons passed: {checks}", flush=True)


if __name__ == "__main__":
    main()
