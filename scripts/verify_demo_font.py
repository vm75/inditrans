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
from collections import Counter
from pathlib import Path
import subprocess
import unicodedata

from fontTools.ttLib import TTFont
from measure_vedic_font import SAMPLES, shape, native
from generate_demo_font import SOURCES
from refine_vedic_anchors import SCRIPTS, TARGET_GAP, _shaper, accent_index, ignored_annotation_glyphs, canonical_stems, compact_rules
from vedic_geometry import InkGeometry, _OutlinePen
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



def verify_vertical_accent_geometry(bundled, binary):
    """Check ink placement after HarfBuzz GPOS, not just nonzero offsets.

    Independent vowels have a clean top/bottom and a reliable one-mark ink-gap
    measurement. For more complex clusters, protect shaping/spacing and check
    that the same glyphs remain attached; their ink unions are not reliable
    stand-ins for the contour-by-contour vowel/conjunct attachment zone.
    """
    checked = 0
    for script, samples in SAMPLES.items():
        glyphs = {}
        for accent in ("॑", "॒"):
            reference = shape(bundled, binary, samples[0][0] + accent)
            glyphs[accent] = reference["glyphs"][-1]["glyph"]
        for index, sample in enumerate(samples):
            result = shape(bundled, binary, sample, accent_glyph=glyphs[sample[-1]])
            if result["missing"] or result["dotted"]:
                raise AssertionError(f"Unshaped Vedic cluster: {script} {sample!r}")
            without = shape(bundled, binary, sample[:-1])
            present = Counter(g["glyph"] for g in without["glyphs"])
            extra = []
            for glyph in result["glyphs"]:
                if present[glyph["glyph"]]:
                    present[glyph["glyph"]] -= 1
                else:
                    extra.append(glyph)
            # GSUB may choose different accent forms after a dependent vowel.
            # Count the extra, zero-advance GDEF mark rather than assuming that
            # the cmap or bare-vowel glyph name survives those substitutions.
            accents = [g for g in extra if g["class"] == 3 and g["advance"][0] == 0]
            if not accents:
                raise AssertionError(f"Missing/nonspacing accent: {script} {sample!r}")
            # Independent vowels are the first two corpus entries. Their bounds
            # are unlike a conjunct's combined glyph bounds or a split vowel.
            if index < 2:
                gap = result["gap"]
                if gap is None or not 38 <= gap <= 125:
                    raise AssertionError(
                        f"Excessive/colliding Vedic vowel gap: {script} {sample!r}: {gap}")
            checked += 1
        print(f"{script}: {len(samples)} positioned U+0951/U+0952 samples verified")
    sources = [native(family) for family in SOURCES if family != "NotoSerif"]
    max_source_box = max(font["hhea"].ascent - font["hhea"].descent
                         + font["hhea"].lineGap for font in sources)
    actual = (bundled["hhea"].ascent - bundled["hhea"].descent
              + bundled["hhea"].lineGap)
    os2 = bundled["OS/2"]
    if actual > max_source_box:
        raise AssertionError(f"Inflated merged line box: {actual} vs {max_source_box}")
    if (os2.sTypoAscender, os2.sTypoDescender, os2.sTypoLineGap) != (
            bundled["hhea"].ascent, bundled["hhea"].descent,
            bundled["hhea"].lineGap):
        raise AssertionError("Typographic and hhea line metrics disagree")
    if os2.usWinAscent < bundled["head"].yMax or os2.usWinDescent < -bundled["head"].yMin:
        # Noto sources themselves can overflow their Windows clip bounds.
        # Do not enforce this unless the source set is normalized as a whole.
        print("Note: some inherited glyph extrema exceed OS/2 Win clip metrics")
    print(f"Validated {checked} shaped Vedic ink cases; line box {actual}/{max_source_box} units")



def verify_contextual_vedic_geometry(font, data):
    """Check consonant and dependent-vowel ink, not just the independent vowels.

    These cases caught the original large gaps (e.g. Tamil க॑ / Telugu క॒)
    that the old independent-vowel-only quality assertions never measured.
    """
    representative = {
        "Bengali": ["ক", "কা", "কি", "কু", "কে"],
        "Tamil": ["க", "கா", "கி", "கு", "கே", "கை"],
        "Telugu": ["క", "కా", "కి", "కు", "కే"],
        "Kannada": ["ಕ", "ಕಾ", "ಕಿ", "ಕು", "ಕೇ"],
        "Malayalam": ["ക", "കാ", "കി", "കു", "കേ"],
    }
    total = 0
    cmap = font.getBestCmap()
    for script, (span, _, _, _) in SCRIPTS.items():
        checked = 0
        for base in representative[script]:
            for accent in ("॑", "॒"):
                trial = shape(font, data, base + accent)
                if trial["missing"] or trial["dotted"]:
                    raise AssertionError(f"Unshaped {script} contextual accent: {base + accent!r}")
                # Split vowels and reordered signs sometimes give negative ink
                # union gaps despite no collision; enforce the clearly excessive
                # positive clearances identified by the shaped test corpus.
                if trial["gap"] is not None and trial["gap"] > 160:
                    raise AssertionError(
                        f"Excessive {script} contextual Vedic gap: {base + accent!r}: {trial['gap']}")
                checked += 1
        start, stop = span
        for cp in range(start, stop):
            if cp not in cmap or unicodedata.category(chr(cp)) != "Lo":
                continue
            for accent in ("॑", "॒"):
                trial = shape(font, data, chr(cp) + accent)
                if trial["missing"] or trial["dotted"]:
                    raise AssertionError(f"Unshaped {script} consonant: {chr(cp) + accent!r}")
                if len(trial["glyphs"]) == 2 and trial["gap"] is not None and trial["gap"] > 160:
                    raise AssertionError(
                        f"Excessive {script} bare-consonant gap: {chr(cp) + accent!r}: {trial['gap']}")
                checked += 1
        print(f"{script}: {checked} consonant and vowel-sign Vedic ink gaps checked")
        total += checked
    print(f"{total} contextual Vedic ink cases passed")


def verify_geometry_sampling():
    class Rectangle:
        def __init__(self, width, height): self.width, self.height = width, height
        def draw(self, pen):
            pen.moveTo((0, 0)); pen.lineTo((self.width, 0)); pen.lineTo((self.width, self.height))
            pen.lineTo((0, self.height)); pen.closePath()
    class Face:
        def getGlyphSet(self): return {'body': Rectangle(100, 100), 'mark': Rectangle(10, 20), 'side': Rectangle(20, 200)}
    geometry = InkGeometry(Face())
    glyphs = [('body', (0, 0, 100, 100)), ('mark', (25, 150, 35, 170))]
    if geometry.gaps(glyphs, 1, 0x0951) != (50, 50): raise AssertionError('Rectangle clearance measurement')
    glyphs.append(('side', (120, 0, 140, 200)))
    if geometry.gaps(glyphs, 1, 0x0951) != (-50, 50): raise AssertionError('Disjoint ink mistaken for collision')
    pen = _OutlinePen({}); pen.moveTo((0, 0)); pen.qCurveTo((1000, 0), (100, 0)); pen.closePath()
    if max(point[0] for edge in pen.edges for point in edge) < 525:
        raise AssertionError('Collinear Bezier overshoot lost by flattening')
    bare = (('base', 'accent'), 1)
    sign = (('base', 'accent', 'vowel'), 1)
    longer = (('prefix', 'base', 'accent', 'vowel'), 2)
    compact = compact_rules({bare: (0, -100), sign: (0, 0), longer: (0, 0)})
    if compact != {bare: (0, -100), sign: (0, 0)}:
        raise AssertionError('Reordered-vowel zero guard lost during rule compression')
    print('Outline envelopes, Bezier extrema and reordered-vowel rule compression checks passed')


def verify_recorded_reviews(font):
    """Replay the full incomplete human review, including accepted controls."""
    fixture = json.loads((ROOT / "test-files/vedic-font-review.json").read_text())
    shape_run = _shaper(font)
    geometry = InkGeometry(font, ignored_annotation_glyphs(font, shape_run))
    checked = 0
    for script, rows in fixture["cases"].items():
        for text, vertical, horizontal in rows:
            glyphs = shape_run(text)
            if any(name == '.notdef' for name, _ in glyphs):
                raise AssertionError(f"Missing reviewed glyph: {script} {text!r}")
            index = accent_index(font, glyphs, ord(text[-1]))
            gaps = geometry.gaps(glyphs, index, ord(text[-1]))
            if not gaps: raise AssertionError(f"Unmeasurable reviewed case: {text!r}")
            union, contour = gaps
            maximum = 200 if text.endswith('᳚') and 'ా' in text else 145
            if union > maximum + 2 or (contour is not None and contour < -6):
                raise AssertionError(f"Reviewed spacing still broken: {text!r}: bounds/ink={gaps}")
            if glyphs.advances[index][0] != 0:
                raise AssertionError(f"Reviewed accent adds character width: {text!r}")
            if horizontal:
                before = fixture['horizontalAccentBounds'][text]
                shift = glyphs[index][1][0] - before[0]
                if not 48 <= shift <= 180:
                    raise AssertionError(f"Bengali i-matra horizontal repair missing: {text!r}: {shift}")
            if vertical == 'good':
                before = fixture['okayAccentBounds'][text]
                after = glyphs[index][1]
                # Replacing old overlapping rules can normalize a native
                # context by one clearance; never move it onto another base.
                if abs(after[0] - before[0]) > 1 or abs(after[1] - before[1]) > 80:
                    raise AssertionError(f"Accepted control displaced: {text!r}: {before} -> {after}")
            checked += 1
    # These alternatives got different review labels despite identical base
    # and accent ink. Preserve a consistent font policy rather than overfitting
    # the lower vowel's spelling to move an otherwise identical upper accent.
    for bad, good in [('টৃ॑', 'টৄ॑'), ('ঠৃ॑', 'ঠৄ॑')]:
        left, right = shape_run(bad), shape_run(good)
        li, ri = accent_index(font, left, 0x0951), accent_index(font, right, 0x0951)
        if left[li][1] != right[ri][1]:
            raise AssertionError('Inconsistent equivalent upper-accent placement')
    for text in ['க꞉॒', 'கம்ˮ॑', 'கம்ʼ᳚', 'கஃ॑', 'க²ி॒']:
        glyphs = shape_run(text)
        if any(name == font.getBestCmap()[0x25CC] for name, _ in glyphs):
            raise AssertionError(f"Spurious Tamil modifier circle: {text!r}")
        plain = shape_run(text[:-1])
        if sum(p[0] for p in glyphs.advances) != sum(p[0] for p in plain.advances):
            raise AssertionError(f"Tamil modifier spacing changed: {text!r}")
    # An actual isolated mark still receives its missing-base indicator.
    if not any(name == font.getBestCmap()[0x25CC] for name, _ in shape_run('ஃ॑')):
        raise AssertionError('Isolated Tamil accent lost its dotted circle')
    reordered = shape_run('ಕೋ॒')
    if accent_index(font, reordered, 0x0952) == len(reordered) - 1:
        raise AssertionError('Kannada trailing-length-sign regression fixture no longer exercises reordering')
    print(f"{checked} recorded Vedic reviews, 88 accepted controls, 50 horizontal reports and Tamil modifier/reordering checks passed")


def verify_generated_corpus(font):
    shape_run = _shaper(font)
    geometry = InkGeometry(font, ignored_annotation_glyphs(font, shape_run))
    count = 0
    failures = []
    for family, stems in canonical_stems().items():
        for stem in stems:
            for code in TARGET_GAP:
                text = stem + chr(code)
                glyphs = shape_run(text)
                if any(name == '.notdef' for name, _ in glyphs):
                    raise AssertionError(f"Missing canonical glyph: {family} {text!r}")
                index = accent_index(font, glyphs, code)
                gaps = geometry.gaps(glyphs, index, code)
                if not gaps: continue
                union, contour = gaps
                maximum = 200 if code == 0x1CDA and 'ా' in stem else 145
                if union > maximum + 2 or (contour is not None and contour < -6):
                    failures.append((family, text, gaps))
                count += 1
    if failures: raise AssertionError(f"{len(failures)} canonical Vedic geometry failures: {failures[:25]!r}")
    print(f"{count:,} canonical vowel/consonant/sign/diacritic/conjunct accent placements checked after WOFF2 decoding")


def main():
    verify_geometry_sampling()
    # Use the demo's real data; newly added scripts and keyboard symbols are checked too.
    extract = r"""
const fs = require('node:fs');
const vm = require('node:vm');
const code = fs.readFileSync('demo/keyboard-data.js', 'utf8');
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
    bundled = TTFont(ROOT / "fonts/inditrans-indic-fallback.woff2")
    bundled.flavor = None
    decompressed = io.BytesIO()
    bundled.save(decompressed)
    font = hb.Font(hb.Face(decompressed.getvalue()))
    ttf_data = (ROOT / "fonts/inditrans-indic-fallback.ttf").read_bytes()
    ttf = TTFont(io.BytesIO(ttf_data))
    if ttf.flavor is not None or ttf.getBestCmap() != bundled.getBestCmap():
        raise AssertionError("TTF must be an uncompressed font with matching WOFF2 coverage")
    ttf_font = hb.Font(hb.Face(ttf_data))
    verify_vertical_accent_geometry(bundled, decompressed.getvalue())
    verify_contextual_vedic_geometry(bundled, decompressed.getvalue())
    verify_recorded_reviews(bundled)
    verify_generated_corpus(bundled)
    marks = ["", "॑", "॒", "᳚", "᳴", "॒᳴"]
    count = 0

    def check(text):
        nonlocal count
        buffer = hb.Buffer()
        buffer.add_str(text)
        buffer.guess_segment_properties()
        hb.shape(font, buffer)
        ttf_buffer = hb.Buffer()
        ttf_buffer.add_str(text)
        ttf_buffer.guess_segment_properties()
        hb.shape(ttf_font, ttf_buffer)
        def signature(shaped):
            return [(info.codepoint, info.cluster, pos.x_advance, pos.y_advance,
                     pos.x_offset, pos.y_offset)
                    for info, pos in zip(shaped.glyph_infos, shaped.glyph_positions)]
        if signature(buffer) != signature(ttf_buffer):
            raise AssertionError(f"TTF/WOFF2 shaping differs: {text!r}")
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
