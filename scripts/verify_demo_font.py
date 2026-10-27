#!/usr/bin/env python3
# /// script
# requires-python = ">=3.10"
# dependencies = ["fonttools[woff]==4.67.0", "uharfbuzz==0.57.0"]
# ///
"""Check complete Indic clusters, not just isolated code points or font downloads."""

import io
import hashlib
from urllib.request import urlopen
import json
from pathlib import Path
import subprocess

from fontTools.ttLib import TTFont
import uharfbuzz as hb

ROOT = Path(__file__).resolve().parents[1]

BUNDLED_SCRIPTS = {
    "assamese", "bengali", "devanagari", "gujarati", "gurmukhi", "kannada",
    "malayalam", "oriya", "sinhala", "tamil", "telugu",
}


REFERENCE_FONTS = {
    "urdu": ("NotoNastaliqUrdu", "2fd7b78ce48768b4d7b251457191e0f6db4c85e966a8a10a1f04e196dad35bb6"),
    "tibetan": ("NotoSerifTibetan", "3dd3876c670008cef5e29a79392fc8874d9bb9e349fc54755b2861c050c0fd7c"),
    "burmese": ("NotoSerifMyanmar", "be851bee6c1dad293c176473a62b61f59bbc516bd0e238e7db2b19be5f69e0f8"),
    "thai": ("NotoSerifThai", "36718303c18459aa1a6010c71566b1487b4385e923cb13e367ff7e7fba218ae0"),
    "khmer": ("NotoSerifKhmer", "97f00cabc9f55ec1fc74e1e7d84e92e2fdd260a641b31313e8d99b136a289424"),
}


def verify_reference_fonts(roman_font):
    subprocess.run(["make", "out/inditrans"], cwd=ROOT, check=True)
    # Forward engine output, with native/contextual forms covered by shared tests.
    script_data = json.loads((ROOT / "scripts/script_data.json").read_text())
    inventory = script_data["roman"]["iso"]
    source = " ".join(inventory["vowels"] + [c + "a" for c in inventory["consonants"]]
                      + ["a" + mark for mark in inventory["otherDiacritics"]]
                      + ["ki", "kī", "ku", "kū", "kē", "krē", "kka", "kta", "khā", "śrī", "kaī", "āē", "kqa"])
    for script, (family, digest) in REFERENCE_FONTS.items():
        path = ROOT / "out/demo-font-sources" / (family + "-Regular.ttf")
        if not path.exists():
            path.parent.mkdir(parents=True, exist_ok=True)
            url = f"https://raw.githubusercontent.com/notofonts/noto-fonts/ffebf8c1ee449e544955a7e813c54f9b73848eac/hinted/ttf/{family}/{path.name}"
            with urlopen(url, timeout=30) as response:
                content = response.read()
            if hashlib.sha256(content).hexdigest() != digest:
                raise AssertionError(f"Font source hash mismatch: {family}")
            path.write_bytes(content)
        content = path.read_bytes()
        if hashlib.sha256(content).hexdigest() != digest:
            raise AssertionError(f"Cached font source hash mismatch: {family}")
        font = hb.Font(hb.Face(content))
        text = subprocess.check_output([str(ROOT / "out/inditrans"), "-f", "iso", "-t", script, source], text=True).strip()
        for syllable in text.split():
            buffer = hb.Buffer()
            buffer.add_str(syllable)
            buffer.guess_segment_properties()
            hb.shape(font, buffer)
            if any(g.codepoint == 0 for g in buffer.glyph_infos):
                raise AssertionError(f"Missing glyph in {family}: {syllable!r}")
        # Verify actual conjunct/joining substitutions, not just character coverage.
        sample = {"urdu": "کِی", "tibetan": "ཀྐ", "burmese": "က္က", "khmer": "ក្ក", "thai": "เก"}[script]
        buffer = hb.Buffer(); buffer.add_str(sample); buffer.guess_segment_properties(); hb.shape(font, buffer)
        face = TTFont(path)
        circle = face.getBestCmap().get(0x25CC)
        if circle and any(g.codepoint == face.getGlyphID(circle) for g in buffer.glyph_infos):
            raise AssertionError(f"Unexpected dotted circle: {family}: {sample!r}")
        print(f"{script}: {len(text.split())} raw engine syllables shaped with pinned {family}")


    for script in script_data["roman"]:
        if script == "iso":
            texts = [subprocess.check_output([str(ROOT / "out/inditrans"), "-f", "devanagari", "-t", "iso", "ऍ कॅ ᳵ ᳶ"], text=True),
                     subprocess.check_output([str(ROOT / "out/inditrans"), "-f", "urdu", "-t", "iso", "ثَ صَ حَ طَ عَ ذَ ضَ ظَ ژَ"], text=True)]
        else:
            texts = [subprocess.check_output([str(ROOT / "out/inditrans"), "-f", "iso", "-t", script, source], text=True)]
        for text in texts:
            buffer = hb.Buffer(); buffer.add_str(text.strip()); buffer.guess_segment_properties(); hb.shape(roman_font, buffer)
            if any(g.codepoint == 0 for g in buffer.glyph_infos):
                raise AssertionError(f"Missing glyph in Roman profile {script}: {text!r}")
        print(f"{script}: forward Roman output has complete bundled font coverage")


def main():
    # Use the demo's real data; newly added scripts and keyboard symbols are checked too.
    extract = r"""
const fs = require('node:fs');
const vm = require('node:vm');
const html = fs.readFileSync('demo/index.html', 'utf8');
const code = html.slice(html.indexOf('const KEYBOARD_MAPS'), html.indexOf('const FONT_RESOURCES'));
const data = vm.runInNewContext(code + ';({ maps: KEYBOARD_MAPS, scripts: SCRIPT_CATALOG })');
const rawCases = [];
const telugu = data.maps.telugu;
for (const base of ['', 'క', 'క్య', 'క్ష్య']) {
  const vowels = base ? telugu.vowelMarks.slice(1, 16) : telugu.vowels.slice(0, 16);
  for (const vowel of vowels) {
    for (const accent of ['॑', '॒', '᳚']) rawCases.push([base + vowel + accent, base + vowel]);
  }
}
console.log(JSON.stringify({scripts: Object.fromEntries(Object.entries(data.scripts)
  .filter(([, script]) => script.kind === 'indic').map(([id]) => [id, data.maps[id]])), rawCases}));
"""
    data = json.loads(subprocess.check_output(["node", "-e", extract], cwd=ROOT, text=True))
    # Other catalog entries use hosted/reference fonts, not the bundled face.
    corpus = {script: layout for script, layout in data["scripts"].items() if script in BUNDLED_SCRIPTS}
    bundled = TTFont(ROOT / "demo/fonts/inditrans-indic-fallback.woff2")
    bundled.flavor = None
    decompressed = io.BytesIO()
    bundled.save(decompressed)
    font = hb.Font(hb.Face(decompressed.getvalue()))
    marks = ["", "॑", "॒", "᳚", "᳴", "॒᳴"]
    count = 0

    def check(text):
        nonlocal count
        buffer = hb.Buffer()
        buffer.add_str(text)
        buffer.guess_segment_properties()
        hb.shape(font, buffer)
        if any(glyph.codepoint == 0 for glyph in buffer.glyph_infos):
            raise AssertionError(f"Missing glyph after shaping: {text!r}")
        count += 1
        return buffer

    dotted_circle = bundled.getGlyphID(bundled.getBestCmap()[0x25CC])
    circles = lambda shaped: sum(glyph.codepoint == dotted_circle for glyph in shaped.glyph_infos)
    width = lambda shaped: sum(position.x_advance for position in shaped.glyph_positions)

    def accent_positions(shaped, length):
        pen = 0
        positions = []
        for glyph, position in zip(shaped.glyph_infos, shaped.glyph_positions):
            positions.append((glyph.codepoint, pen + position.x_offset, position.y_offset))
            pen += position.x_advance
        return positions[-length:]

    annotation_cases = 0
    for base in ["க", "கி", "கு", "கீ", "க்ஷ", "த்யே", "ஃக"]:
        for digit in "²³⁴":
            for accent in marks[1:]:
                reference = check(base + accent)
                annotated = check(base + digit + accent)
                if circles(annotated) or width(annotated) != width(check(base + digit)):
                    raise AssertionError(f"Broken Tamil annotation cluster: {base + digit + accent!r}")
                # Glyph existence is insufficient: the accent must attach to the
                # same Tamil base as it does without the annotation, not the digit.
                if accent_positions(annotated, len(accent)) != accent_positions(reference, len(accent)):
                    raise AssertionError(f"Detached Tamil annotation accent: {base + digit + accent!r}")
                annotation_cases += 1
    print(f"{annotation_cases} Tamil annotation cases preserve accent attachment and character spacing")

    oriya_cases = ["ଗ॒", "ଗ॑", "ଗୁ॒", "ଗୀ॑", "ଗ୍ଗ॒", "ଗ୍ୟ॑", "କ୍ଷ॒", "କ୍ଷୀ॑"]
    for text in oriya_cases:
        accented = check(text)
        native = check(text.rstrip("॒॑"))
        if circles(accented) or width(accented) != width(native):
            raise AssertionError(f"Broken Oriya accent cluster: {text!r}")
    print(f"{len(oriya_cases)} Oriya accent cases add no dotted circles or spacing")

    for script, layout in corpus.items():
        for values in layout.values():
            for value in values:
                if value:
                    for mark in marks:
                        check(value + mark)
        for consonant in layout["consonants"]:
            if not consonant:
                continue
            # Conjuncts plus dependent vowels and stacked accents exercise whole clusters.
            conjunct = consonant + layout["vowelMarks"][0] + layout["consonants"][25]
            for vowel in layout["vowelMarks"]:
                for mark in marks:
                    check(conjunct + vowel + mark)
        print(f"{script}: keyboard characters, conjuncts, vowels and accents passed")
    for text in ["த்யெ॒᳴", "த்யே॒᳴", "த்யா॒᳴", "ఎ॒", "ఏ॑", "ఒ᳚", "ਧ੍ਯ᳚"]:
        check(text)
    # A glyph can exist yet float at its source font's origin. Require explicit
    # nonspacing attachment on scripts borrowing U+1CF4 (the iPhone report).
    for text in ["அ᳴", "ఎ᳴", "ఏ᳴", "ఒ᳴", "ਅ᳴", "অ᳴", "અ᳴", "അ᳴", "ଅ᳴", "අ᳴"]:
        buffer = hb.Buffer()
        buffer.add_str(text)
        buffer.guess_segment_properties()
        hb.shape(font, buffer)
        mark = buffer.glyph_positions[-1]
        if mark.x_advance != 0 or (mark.x_offset == 0 and mark.y_offset == 0):
            raise AssertionError(f"Unattached borrowed Vedic mark: {text!r}")
    for text, native in data["rawCases"]:
        buffer = check(text)
        reference = check(native)
        if circles(buffer) != circles(reference) or width(buffer) != width(reference):
            raise AssertionError(f"Broken raw Telugu accent cluster: {text!r}")
    print(f"{len(data['rawCases'])} raw Telugu accent cases add no dotted circles or spacing")
    print("Borrowed Vedic marks attach without adding character spacing")
    print(f"Passed {count:,} shaped strings across {len(corpus)} Indic scripts; no missing glyphs")
    verify_reference_fonts(font)


if __name__ == "__main__":
    main()
