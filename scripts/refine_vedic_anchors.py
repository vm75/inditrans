"""Repair Vedic accent gaps with sparse context-sensitive GPOS adjustments.

A bare consonant and a consonant with a vowel sign can share a base anchor.
We measure their HarfBuzz-shaped ink separately, then add residual ContextPos
adjustments. Longer glyph sequences compensate shorter matches as needed.
Non-Vedic mark classes, GSUB, glyph outlines and advances stay unchanged.
"""
from collections import defaultdict
import io
import unicodedata

from fontTools.otlLib.builder import buildCoverage, buildLookup, buildSinglePosSubtable, buildValue
from fontTools.pens.boundsPen import BoundsPen
from fontTools.ttLib.tables import otTables
import uharfbuzz as hb

# consonant range, dependent-mark range, virama, OpenType script tags
SCRIPTS = {
    "Bengali": ((0x0995, 0x09BA), (0x09BC, 0x09E3), 0x09CD, ("beng", "bng2")),
    "Tamil": ((0x0B95, 0x0BBA), (0x0BBE, 0x0BD0), 0x0BCD, ("taml", "tml2")),
    "Telugu": ((0x0C15, 0x0C3A), (0x0C3E, 0x0C63), 0x0C4D, ("telu", "tel2")),
    "Kannada": ((0x0C95, 0x0CBA), (0x0CBE, 0x0CE3), 0x0CCD, ("knda", "knd2")),
    "Malayalam": ((0x0D15, 0x0D3A), (0x0D3E, 0x0D63), 0x0D4D, ("mlym", "mlm2")),
}
FEATURE = {
    "Bengali": {0x0951: "blwm", 0x0952: "blwm"},
    "Tamil": {0x0951: "abvm", 0x0952: "blwm"},
    "Telugu": {0x0951: "abvm", 0x0952: "abvm"},
    "Kannada": {0x0951: "abvm", 0x0952: "abvm"},
    "Malayalam": {0x0951: "blwm", 0x0952: "blwm"},
}
TARGET_GAP = {0x0951: 64, 0x0952: 76}
EXCESS_GAP = 145


def _shaper(font):
    stream = io.BytesIO()
    font.save(stream)
    hb_font = hb.Font(hb.Face(stream.getvalue()))
    order = font.getGlyphOrder()
    glyphs = font.getGlyphSet()
    bounds = {}

    def shape(text):
        buffer = hb.Buffer()
        buffer.add_str(text)
        buffer.guess_segment_properties()
        hb.shape(hb_font, buffer)
        x = y = 0
        result = []
        for info, pos in zip(buffer.glyph_infos, buffer.glyph_positions):
            name = order[info.codepoint]
            if name not in bounds:
                pen = BoundsPen(glyphs)
                glyphs[name].draw(pen)
                bounds[name] = pen.bounds
            rect = bounds[name]
            if rect:
                rect = (rect[0] + x + pos.x_offset, rect[1] + y + pos.y_offset,
                        rect[2] + x + pos.x_offset, rect[3] + y + pos.y_offset)
            result.append((name, rect))
            x += pos.x_advance
            y += pos.y_advance
        return result

    return shape


def _ink_gap(glyphs, mark_index, mark):
    rect = glyphs[mark_index][1]
    others = [r for index, (_, r) in enumerate(glyphs) if index != mark_index and r]
    if not rect or not others:
        return None
    return (rect[1] - max(r[3] for r in others)) if mark == 0x0951 else (
        min(r[1] for r in others) - rect[3])


def _cases(font, script):
    (lo, hi), (start, stop), virama, _ = SCRIPTS[script]
    cmap = font.getBestCmap()
    letters = [cp for cp in range(lo, hi) if cp in cmap
               and unicodedata.category(chr(cp)) == "Lo"]
    signs = [cp for cp in range(start, stop) if cp in cmap and cp != virama
             and unicodedata.category(chr(cp)) in ("Mn", "Mc")]
    for cp in letters:
        root = chr(cp)
        stems = [root] + [root + chr(sign) for sign in signs]
        for next_cp in (letters[0], letters[len(letters) // 2]):
            cluster = root + chr(virama) + chr(next_cp)
            stems.append(cluster)
            stems.extend(cluster + chr(sign) for sign in signs)
        for stem in stems:
            for mark in TARGET_GAP:
                yield stem + chr(mark), mark
                if script == "Tamil":
                    for digit in "²³⁴":
                        yield stem + digit + chr(mark), mark


def _install(font, script, rules):
    if not rules:
        return 0
    gpos = font["GPOS"].table
    glyph_map = font.getReverseGlyphMap()

    def lookup(subtables):
        index = len(gpos.LookupList.Lookup)
        gpos.LookupList.Lookup.append(buildLookup(subtables))
        return index

    def context(glyphnames, accent_index, shift_lookup):
        table = otTables.ContextPos()
        table.Format = 3
        table.GlyphCount = len(glyphnames)
        table.PosCount = 1
        table.Coverage = [
            buildCoverage([name], glyph_map) for name in glyphnames
        ]
        record = otTables.PosLookupRecord()
        record.SequenceIndex = accent_index
        record.LookupListIndex = shift_lookup
        table.PosLookupRecord = [record]
        return table

    count = 0
    for tag in ("abvm", "blwm"):
        by_length = defaultdict(list)
        for (feature, names, index, mark), delta in rules.items():
            if feature == tag and delta:
                by_length[len(names)].append((names, index, delta))
        if not by_length:
            continue
        shift_lookups = {}
        contexts = []
        for size in sorted(by_length):
            subtables = []
            for names, index, delta in by_length[size]:
                key = (names[index], delta)
                if key not in shift_lookups:
                    adjustment = buildSinglePosSubtable(
                        {key[0]: buildValue({"YPlacement": delta})}, glyph_map)
                    shift_lookups[key] = lookup([adjustment])
                subtables.append(context(names, index, shift_lookups[key]))
            # Bound lookup subtable arrays to keep OpenType 16-bit offsets valid.
            for i in range(0, len(subtables), 80):
                contexts.append(lookup(subtables[i:i + 80]))
        gpos.LookupList.LookupCount = len(gpos.LookupList.Lookup)
        attached = set()
        for script_record in gpos.ScriptList.ScriptRecord:
            if script_record.ScriptTag not in SCRIPTS[script][3]:
                continue
            languages = [script_record.Script.DefaultLangSys] + [
                entry.LangSys for entry in script_record.Script.LangSysRecord]
            for language in filter(None, languages):
                for feature_index in language.FeatureIndex:
                    feature = gpos.FeatureList.FeatureRecord[feature_index]
                    if feature.FeatureTag == tag and feature_index not in attached:
                        feature.Feature.LookupListIndex.extend(contexts)
                        feature.Feature.LookupCount = len(feature.Feature.LookupListIndex)
                        attached.add(feature_index)
        if not attached:
            raise ValueError(f"Missing {script} {tag} native GPOS feature")
        count += sum(map(len, by_length.values()))
    return count


def repair_contextual_vedic_anchors(font):
    """Measure consonant/matra contexts and install only needed Vedic offsets."""
    shape = _shaper(font)
    stats = {}
    for script in SCRIPTS:
        sample_letter = chr(SCRIPTS[script][0][0])
        accent_names = {
            mark: shape(sample_letter + chr(mark))[-1][0] for mark in TARGET_GAP
        }
        measured = {}
        for text, mark in _cases(font, script):
            glyphs = shape(text)
            if not 2 <= len(glyphs) <= 7 or any(name == ".notdef" for name, _ in glyphs):
                continue
            matches = [i for i, (name, _) in enumerate(glyphs)
                       if name == accent_names[mark]]
            if len(matches) != 1:
                continue
            mark_index = matches[0]
            gap = _ink_gap(glyphs, mark_index, mark)
            if gap is None:
                continue
            # Tamil annotation glyphs should not change accent attachment:
            # retain the SAME positional correction as the unannotated syllable
            # even if the extra digit makes the ink-union gap appear small.
            measured_gap = gap
            if script == "Tamil" and any(digit in text for digit in "²³⁴"):
                clean = text.translate(str.maketrans("", "", "²³⁴"))
                clean_glyphs = shape(clean)
                clean_indices = [i for i, (name, _) in enumerate(clean_glyphs)
                                 if name == accent_names[mark]]
                if len(clean_indices) == 1:
                    measured_gap = _ink_gap(clean_glyphs, clean_indices[0], mark)
            desired = 0
            if measured_gap is not None and measured_gap > EXCESS_GAP:
                desired = round(TARGET_GAP[mark] - measured_gap) if mark == 0x0951 else (
                    round(measured_gap - TARGET_GAP[mark]))
            key = (FEATURE[script][mark], tuple(name for name, _ in glyphs),
                   mark_index, mark)
            measured[key] = desired

        rules = {}
        for key, desired in sorted(measured.items(), key=lambda pair: len(pair[0][1])):
            tag, sequence, accent_index, mark = key
            # Short lookup matches inside longer Indic clusters. Compensate
            # their shifts with residual longer-context rules; this preserves
            # vowel-sign/conjunct examples that already have adequate gaps.
            inherited = 0
            for (prior_tag, pattern, prior_index, prior_mark), delta in rules.items():
                if prior_tag != tag or prior_mark != mark:
                    continue
                for start in range(len(sequence) - len(pattern) + 1):
                    if (sequence[start:start + len(pattern)] == pattern
                            and start + prior_index == accent_index):
                        inherited += delta
            residual = desired - inherited
            if residual:
                rules[key] = residual
        stats[script] = {
            "measured": len(measured),
            "context_rules": _install(font, script, rules),
        }
    return stats
