#!/usr/bin/env python3
# /// script
# requires-python = ">=3.10"
# dependencies = ["fonttools[woff]==4.67.0", "uharfbuzz==0.57.0"]
# ///
"""Check complete Indic clusters, not just isolated code points or font downloads."""

import io
import json
from pathlib import Path
import subprocess

from fontTools.ttLib import TTFont
import uharfbuzz as hb

ROOT = Path(__file__).resolve().parents[1]


def main():
    # Use the demo's real data; newly added scripts and keyboard symbols are checked too.
    extract = r"""
const fs = require('node:fs');
const vm = require('node:vm');
const html = fs.readFileSync('demo/index.html', 'utf8');
const code = html.slice(html.indexOf('const KEYBOARD_MAPS'), html.indexOf('const FONT_RESOURCES'));
const data = vm.runInNewContext(code + ';({ maps: KEYBOARD_MAPS, scripts: SCRIPT_CATALOG })');
const display = vm.runInNewContext(html.slice(html.indexOf('function displayIndicText'),
  html.indexOf('function createEditHistory')) + ';displayIndicText');
const displayCases = [];
const telugu = data.maps.telugu;
const bases = telugu.consonants.flatMap(c=>[c, c + telugu.vowelMarks[0] + telugu.consonants[25]]);
for (const [base, vowels] of [['', telugu.vowels], ...bases.map(base=>[base, telugu.vowelMarks])]) {
  for (const vowel of vowels.filter(v=>v.includes('\u200b'))) {
    for (const accent of ['॑', '॒', '᳚']) displayCases.push(display(base + vowel + accent));
  }
}
console.log(JSON.stringify({scripts: Object.fromEntries(Object.entries(data.scripts)
  .filter(([, script]) => script.kind === 'indic').map(([id]) => [id, data.maps[id]])), displayCases}));
"""
    data = json.loads(subprocess.check_output(["node", "-e", extract], cwd=ROOT, text=True))
    corpus = data["scripts"]
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
    for text in ["த்யெ॒᳴", "த்யே॒᳴", "த்யா॒᳴", "ఎ᳴\u200b", "ఏ᳴", "ఒ᳴\u200b", "ਧ੍ਯ᳚"]:
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
    dotted_circle = bundled.getGlyphID(bundled.getBestCmap()[0x25CC])
    for text in data["displayCases"]:
        buffer = check(text)
        reference = check(text.replace("\u200b", ""))
        circles = lambda shaped: sum(glyph.codepoint == dotted_circle for glyph in shaped.glyph_infos)
        width = lambda shaped: sum(position.x_advance for position in shaped.glyph_positions)
        if circles(buffer) != circles(reference) or width(buffer) != width(reference):
            raise AssertionError(f"Broken Telugu accent cluster after display adjustment: {text!r}")
    print(f"{len(data['displayCases'])} Telugu separator/accent cases add no dotted circles or spacing")
    print("Borrowed Vedic marks attach without adding character spacing")
    print(f"Passed {count:,} shaped strings across {len(corpus)} Indic scripts; no missing glyphs")


if __name__ == "__main__":
    main()
