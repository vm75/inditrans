"""Build compact, measured positioning rules for complete Vedic syllables.

Rules match at the accent and inspect its shaped backtrack. Longest contexts
win in a single lookup, so a bare-base repair cannot accumulate inside a vowel
or conjunct. Only accent positions change; native outlines and advances remain.
"""
from collections import Counter, defaultdict
import io
import json
from pathlib import Path

from fontTools.otlLib.builder import buildCoverage, buildLookup, buildSinglePosSubtable, buildValue
from fontTools.pens.boundsPen import BoundsPen
from fontTools.ttLib.tables import otTables
import uharfbuzz as hb
from vedic_geometry import InkGeometry

# Kept for the original representative-consonant verifier.
SCRIPTS = {
    "Bengali": ((0x0995, 0x09BA), (0x09BC, 0x09E3), 0x09CD, ("beng", "bng2")),
    "Tamil": ((0x0B95, 0x0BBA), (0x0BBE, 0x0BD0), 0x0BCD, ("taml", "tml2")),
    "Telugu": ((0x0C15, 0x0C3A), (0x0C3E, 0x0C63), 0x0C4D, ("telu", "tel2")),
    "Kannada": ((0x0C95, 0x0CBA), (0x0CBE, 0x0CE3), 0x0CCD, ("knda", "knd2")),
    "Malayalam": ((0x0D15, 0x0D3A), (0x0D3E, 0x0D63), 0x0D4D, ("mlym", "mlm2")),
}
SCRIPT_IDS = {
    "Devanagari": ("devanagari",), "Bengali": ("bengali", "assamese"),
    "Gurmukhi": ("gurmukhi",), "Gujarati": ("gujarati",), "Oriya": ("oriya",),
    "Tamil": ("tamil",), "Telugu": ("telugu",), "Kannada": ("kannada",),
    "Malayalam": ("malayalam",), "Sinhala": ("sinhala",),
}
TARGET_GAP = {0x0951: 64, 0x0952: 76, 0x1CDA: 64}


class GlyphRun(list):
    """Positioned glyph bounds plus original character clusters."""
    pass


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
        result = GlyphRun()
        result.text = text
        result.unaccented = lambda: shape(text[:-1])
        result.advances = [(pos.x_advance, pos.y_advance) for pos in buffer.glyph_positions]
        result.clusters = [info.cluster for info in buffer.glyph_infos]
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



def append_feature_lookups(font, table_tag, script_tags, feature_tag, lookups):
    """Append without replacing native features, then restore sorted indices."""
    table = font[table_tag].table
    patched = set()
    new_index = None
    for record in table.ScriptList.ScriptRecord:
        if record.ScriptTag not in script_tags: continue
        languages = [record.Script.DefaultLangSys] + [entry.LangSys for entry in record.Script.LangSysRecord]
        for language in filter(None, languages):
            found = False
            for index in language.FeatureIndex:
                feature = table.FeatureList.FeatureRecord[index]
                if feature.FeatureTag != feature_tag: continue
                found = True
                if index not in patched:
                    feature.Feature.LookupListIndex.extend(lookups)
                    feature.Feature.LookupCount = len(feature.Feature.LookupListIndex)
                    patched.add(index)
            if not found:
                if new_index is None:
                    new_index = len(table.FeatureList.FeatureRecord)
                    feature = otTables.FeatureRecord(); feature.FeatureTag = feature_tag
                    feature.Feature = otTables.Feature(); feature.Feature.FeatureParams = None
                    feature.Feature.LookupListIndex = list(lookups); feature.Feature.LookupCount = len(lookups)
                    table.FeatureList.FeatureRecord.append(feature)
                    table.FeatureList.FeatureCount = len(table.FeatureList.FeatureRecord)
                language.FeatureIndex.append(new_index)
                language.FeatureCount = len(language.FeatureIndex)
    ordered = sorted(enumerate(table.FeatureList.FeatureRecord), key=lambda item: item[1].FeatureTag)
    remap = {old: new for new, (old, _) in enumerate(ordered)}
    table.FeatureList.FeatureRecord = [record for _, record in ordered]
    for record in table.ScriptList.ScriptRecord:
        for language in filter(None, [record.Script.DefaultLangSys] + [entry.LangSys for entry in record.Script.LangSysRecord]):
            language.FeatureIndex = sorted(remap[index] for index in language.FeatureIndex)
            if language.ReqFeatureIndex != 0xFFFF: language.ReqFeatureIndex = remap[language.ReqFeatureIndex]


def canonical_stems():
    data = json.loads((Path(__file__).with_name("script_data.json")).read_text())["indic"]
    result = {}
    for family, ids in SCRIPT_IDS.items():
        stems = set()
        for script in ids:
            table = data[script]
            vowels, consonants = table["vowels"], table["consonants"]
            signs = ["", *table["vowelMarks"]]
            stems.update(vowels)
            stems.update(c + sign for c in consonants for sign in signs)
            if family == "Tamil":
                for c in consonants:
                    digits = ''.join(char for char in c if char in "²³⁴")
                    if digits:
                        root = ''.join(char for char in c if char not in "²³⁴")
                        stems.update(root + sign + digits for sign in signs)
                for root in ("க", "கி", "கு", "கீ", "க்ஷ", "த்யே", "ஃக"):
                    stems.add(root)
                    stems.update(root + digit for digit in "²³⁴")
            stems.update(base + mark for base in vowels + consonants for mark in table["otherDiacritics"])
            # Representative conjunct structures also protect longer contexts
            # from inheriting the single-consonant rule when geometry differs.
            virama = table["vowelMarks"][0]
            for c in consonants:
                for second in (consonants[0], consonants[25], consonants[26]):
                    stems.update(c + virama + second + sign for sign in signs)
        result[family] = sorted(stems)
    return result


def accent_glyph_names(font, code):
    """Follow GSUB forms; Indic reordering need not leave the accent last."""
    cache = getattr(font, '_vedic_accent_forms', None)
    if cache is None: cache = {}; font._vedic_accent_forms = cache
    if code in cache: return cache[code]
    names = {font.getBestCmap()[code]}
    table = font['GSUB'].table
    # Follow localization, not arbitrary contextual helper substitutions. The
    # latter intentionally reuse mark classes to swap/delete neighbouring
    # diacritics and would incorrectly identify a vowel sign as the accent.
    indices = {i for f in table.FeatureList.FeatureRecord if f.FeatureTag == 'locl' for i in f.Feature.LookupListIndex}
    changed = True
    while changed:
        before = len(names)
        for index in sorted(indices):
            for wrapped in table.LookupList.Lookup[index].SubTable:
                sub = getattr(wrapped, 'ExtSubTable', wrapped)
                for source, target in getattr(sub, 'mapping', {}).items():
                    if source in names and isinstance(target, str): names.add(target)
        changed = len(names) != before
    cache[code] = names
    return names


def accent_index(font, glyphs, code):
    forms = accent_glyph_names(font, code)
    indices = [i for i, (name, _) in enumerate(glyphs) if name in forms]
    if len(indices) == 1: return indices[0]
    if hasattr(glyphs, 'unaccented'):
        present = Counter(name for name, _ in glyphs.unaccented())
        extra = []
        classes = font['GDEF'].table.GlyphClassDef.classDefs
        for i, (name, _) in enumerate(glyphs):
            if present[name]: present[name] -= 1
            elif classes.get(name) == 3 and glyphs.advances[i][0] == 0: extra.append(i)
        if len(extra) == 1: return extra[0]
    raise ValueError(f"Expected one accent U+{code:04X}: {glyphs!r}")


def ignored_annotation_glyphs(font, shape):
    cmap = font.getBestCmap()
    ignored = {cmap[0x25CC]}
    for digit in "²³⁴":
        ignored.add(cmap[ord(digit)])
        ignored.add(shape("க" + digit)[-1][0])
    ignored.update(name for name in font.getGlyphOrder() if name.endswith(".vedicTaml"))
    for char in "ˮʼ꞉": ignored.add(shape("க" + char)[-1][0])
    ignored.add(cmap[0x0B83])
    aytham = shape("கஃ॑")
    ignored.add(aytham[1][0])
    return ignored


def desired_shift(geometry, glyphs, code, index):
    measured = geometry.gaps(glyphs, index, code)
    if not measured: return (0, 0)
    union, contour = measured
    target = TARGET_GAP[code]
    maximum = 200 if code == 0x1CDA and "ా" in glyphs.text else (130 if code == 0x0952 else 145)
    amount = 0
    if union > maximum:
        amount = target - union
    # Do not "repair" a healthy disjoint placement based on box overlap.
    # A small positive contour gap may be intentional in the source font.
    if contour is not None and contour + amount < -4:
        amount = target - contour
    # Four-unit precision retains native placements while sharing shifts.
    dy = 4 * round(amount / 4)
    return (0, int(dy if code != 0x0952 else -dy))


def compact_rules(rules):
    """Drop contexts whose highest-priority retained match has the same shift."""
    compact = {}
    for (names, index), delta in sorted(rules.items(), key=lambda item: (len(item[0][0]), item[0])):
        candidates = []
        # A short rule can match a suffix BEFORE the accent even when the
        # longer context also has a reordered vowel sign AFTER it. Comparing
        # only string suffixes would lose that vital zero-shift guard.
        for back in range(index + 1):
            for ahead in range(len(names) - index):
                key = (names[index - back:index + 1 + ahead], back)
                if key in compact: candidates.append(key)
        inherited = compact[min(candidates, key=lambda key: (-len(key[0]), key[0], key[1]))] if candidates else (0, 0)
        if delta != inherited: compact[(names, index)] = delta
    return compact


def _install_contexts(font, rules):
    table = font["GPOS"].table
    glyph_map = font.getReverseGlyphMap()
    shifts = defaultdict(set)
    for (names, index), delta in rules.items():
        shifts[delta].add(names[index])
    shift_indices = {}
    for delta, names in sorted(shifts.items()):
        values = {name: buildValue({"XPlacement": delta[0], "YPlacement": delta[1]}) for name in sorted(names)}
        shift_indices[delta] = len(table.LookupList.Lookup)
        table.LookupList.Lookup.append(buildLookup([buildSinglePosSubtable(values, glyph_map)], extension=True, table="GPOS"))
    by_mark = defaultdict(list)
    for (names, index), delta in rules.items():
        by_mark[names[index]].append((names, index, delta))
    subtables = []
    for mark in sorted(by_mark, key=glyph_map.get):
        entries = sorted(by_mark[mark], key=lambda item: (-len(item[0]), item[0], item[1]))
        # Offset16 stays within each compact rule set; extension wrappers use
        # Offset32 to reach it. All contexts remain in ONE lookup, longest first.
        for start in range(0, len(entries), 700):
            sub = otTables.ChainContextPos(); sub.Format = 1
            sub.Coverage = buildCoverage([mark], glyph_map)
            rule_set = otTables.ChainPosRuleSet(); rule_set.ChainPosRule = []
            for names, index, delta in entries[start:start + 700]:
                rule = otTables.ChainPosRule()
                rule.Backtrack = list(reversed(names[:index])); rule.BacktrackGlyphCount = index
                # Following vowel signs are context, not positioning input:
                # they may not carry the accent's mkmk feature mask.
                rule.Input = []; rule.InputGlyphCount = 1
                rule.LookAhead = list(names[index + 1:]); rule.LookAheadGlyphCount = len(rule.LookAhead)
                rule.PosLookupRecord = []
                # Explicit zero positioning guards healthy longer contexts
                # against a shorter rule with a nonzero adjustment.
                record = otTables.PosLookupRecord(); record.SequenceIndex = 0
                record.LookupListIndex = shift_indices[delta]; rule.PosLookupRecord.append(record)
                rule.PosCount = len(rule.PosLookupRecord)
                rule_set.ChainPosRule.append(rule)
            rule_set.ChainPosRuleCount = len(rule_set.ChainPosRule)
            sub.ChainPosRuleSet = [rule_set]; sub.ChainPosRuleSetCount = 1
            subtables.append(sub)
    index = len(table.LookupList.Lookup)
    table.LookupList.Lookup.append(buildLookup(subtables, extension=True, table="GPOS"))
    table.LookupList.LookupCount = len(table.LookupList.Lookup)
    return index, len(shifts), len(subtables)


def repair_contextual_vedic_anchors(font, script_tags):
    shape = _shaper(font)
    geometry = InkGeometry(font, ignored_annotation_glyphs(font, shape))
    rules = {}
    samples = {}
    stats = {}
    for script, stems in canonical_stems().items():
        count = 0
        for stem in stems:
            for code in TARGET_GAP:
                glyphs = shape(stem + chr(code))
                if not glyphs or any(name == ".notdef" for name, _ in glyphs): continue
                index = accent_index(font, glyphs, code)
                names = tuple(name for name, _ in glyphs)
                dx, dy = desired_shift(geometry, glyphs, code, index)
                # Bengali's borrowed double stroke used a centered anchor while
                # the native single stroke clears the pre-base i-matra at its
                # left edge. Use that native attachment reference for both
                # strokes, not the bounding-box center of the whole syllable.
                if script == "Bengali" and code == 0x1CDA and "ি" in stem:
                    single = shape(stem + "॑")
                    single_index = accent_index(font, single, 0x0951)
                    single_delta = desired_shift(geometry, single, 0x0951, single_index)
                    # Align the first stroke, and never shift the pair left of
                    # its native attachment on pre-base i forms.
                    pair_width = glyphs[index][1][2] - glyphs[index][1][0]
                    single_width = single[single_index][1][2] - single[single_index][1][0]
                    dx = max(round((pair_width - single_width) / 2), round(single[single_index][1][0] - glyphs[index][1][0]))
                    dy = round(single[single_index][1][1] + single_delta[1] - glyphs[index][1][1])
                    pair_gaps = geometry.gaps(glyphs, index, code, dx=dx)
                    if pair_gaps and pair_gaps[1] is not None and pair_gaps[1] + dy < -4:
                        dy += round(TARGET_GAP[code] - pair_gaps[1] - dy)
                key = (names, index)
                delta = (dx, dy)
                if key in rules and rules[key] != delta:
                    raise ValueError(f"Conflicting shaped context: {stem!r}: {rules[key]} vs {delta}")
                rules[key] = delta
                samples[stem + chr(code)] = (code, delta)
                count += 1
        stats[script] = count
    stats["measured_contexts"] = len(rules)
    rules = compact_rules(rules)
    context_index, shifts, subtables = _install_contexts(font, rules)
    append_feature_lookups(font, "GPOS", {tag for tags in script_tags.values() for tag in tags}, "mkmk", [context_index])
    # Re-shape after compilation: rule compression and overlapping contexts
    # must yield the measured offset, and must NEVER reposition a vowel sign
    # merely because Indic reordering placed it after the accent.
    positioned = _shaper(font)
    for text, (code, delta) in samples.items():
        before, after = shape(text), positioned(text)
        index = accent_index(font, before, code)
        if [name for name, _ in before] != [name for name, _ in after]:
            raise AssertionError(f"Positioning changed glyphs: {text!r}")
        for i, ((_, old_rect), (_, new_rect)) in enumerate(zip(before, after)):
            expected = old_rect
            if old_rect and i == index:
                dx, dy = delta
                expected = (old_rect[0] + dx, old_rect[1] + dy, old_rect[2] + dx, old_rect[3] + dy)
            if new_rect != expected:
                raise AssertionError(f"Wrong contextual placement: {text!r}, glyph {i}: {new_rect} != {expected}; requested {delta}; before {before}")
        if before.advances != after.advances:
            raise AssertionError(f"Positioning changed advances: {text!r}")
    stats.update(verified_clusters=len(samples), context_rules=len(rules), shift_lookups=shifts, context_subtables=subtables)
    return stats
