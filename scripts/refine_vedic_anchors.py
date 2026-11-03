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


def _install_all(font, plans, shared_lookups):
    """Share entire extension-context lookups across script-specific features.

    A merged Indic font already has many native lookups. Adding separate
    context lookups per script overflows GPOS LookupList's 16-bit offsets.
    Lookups can be shared: the coverage glyphs themselves are script-specific.
    """
    gpos = font["GPOS"].table
    glyph_map = font.getReverseGlyphMap()

    def context(names, index, lookup_index):
        table = otTables.ContextPos()
        table.Format = 3
        table.GlyphCount = len(names)
        table.PosCount = 1
        table.Coverage = [buildCoverage([name], glyph_map) for name in names]
        record = otTables.PosLookupRecord()
        record.SequenceIndex = index
        record.LookupListIndex = lookup_index
        table.PosLookupRecord = [record]
        return table

    groups = defaultdict(list)
    for rules in plans.values():
        for (tag, names, index, mark), delta in rules.items():
            if delta:
                groups[(tag, len(names))].append(context(names, index, shared_lookups[delta]))

    feature_lookups = defaultdict(list)
    for (tag, length), subtables in sorted(groups.items()):
        # Offset16 points only to the ExtensionPos wrappers; their data uses
        # Offset32. Limit wrapper arrays to keep the lookup header compact.
        for i in range(0, len(subtables), 1200):
            index = len(gpos.LookupList.Lookup)
            gpos.LookupList.Lookup.append(
                buildLookup(subtables[i:i + 1200], extension=True, table="GPOS"))
            feature_lookups[tag].append(index)
    gpos.LookupList.LookupCount = len(gpos.LookupList.Lookup)

    patched = set()
    for script, (_, _, _, script_tags) in SCRIPTS.items():
        required = {FEATURE[script][mark] for (_, _, _, mark) in plans[script]}
        found = set()
        for script_record in gpos.ScriptList.ScriptRecord:
            if script_record.ScriptTag not in script_tags:
                continue
            languages = [script_record.Script.DefaultLangSys] + [
                entry.LangSys for entry in script_record.Script.LangSysRecord]
            for language in filter(None, languages):
                for feature_index in language.FeatureIndex:
                    feature = gpos.FeatureList.FeatureRecord[feature_index]
                    if feature.FeatureTag not in required:
                        continue
                    found.add(feature.FeatureTag)
                    if feature_index in patched:
                        continue
                    feature.Feature.LookupListIndex.extend(feature_lookups[feature.FeatureTag])
                    feature.Feature.LookupCount = len(feature.Feature.LookupListIndex)
                    patched.add(feature_index)
        if required - found:
            raise ValueError(f"Missing {script} native GPOS features: {required - found}")
    return sum(map(len, groups.values())), sum(map(len, feature_lookups.values()))



def repair_contextual_vedic_anchors(font):
    """Measure consonant/matra contexts and install only needed Vedic offsets."""
    shape = _shaper(font)
    stats = {}
    plans = {}
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
                raw = TARGET_GAP[mark] - measured_gap if mark == 0x0951 else (
                    measured_gap - TARGET_GAP[mark])
                # Quantization bounds the shared GPOS lookup count. A <=4-unit
                # optical tolerance is immaterial at 1000 font units/em.
                desired = 16 * round(raw / 16)
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
        stats[script] = {"measured": len(measured)}
        plans[script] = rules

    # One SinglePos lookup can serve all script-specific accent glyphs sharing
    # the same quantized YPlacement. This avoids the 16-bit GPOS LookupList
    # overflow caused by thousands of one-mark adjustment lookups.
    deltas = defaultdict(set)
    for rules in plans.values():
        for (_, sequence, index, _), amount in rules.items():
            deltas[amount].add(sequence[index])
    gpos = font["GPOS"].table
    glyph_map = font.getReverseGlyphMap()
    shared_lookups = {}
    for amount, names in sorted(deltas.items()):
        mapping = {name: buildValue({"YPlacement": amount}) for name in names}
        subtable = buildSinglePosSubtable(mapping, glyph_map)
        shared_lookups[amount] = len(gpos.LookupList.Lookup)
        gpos.LookupList.Lookup.append(buildLookup([subtable]))
    gpos.LookupList.LookupCount = len(gpos.LookupList.Lookup)

    for script, rules in plans.items():
        stats[script]["context_rules"] = len(rules)
    total, shared_contexts = _install_all(font, plans, shared_lookups)
    stats["shared_shift_lookups"] = len(shared_lookups)
    stats["shared_context_lookups"] = shared_contexts
    stats["total_context_rules"] = total
    return stats
