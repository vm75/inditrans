#!/usr/bin/env python3
# /// script
# requires-python = ">=3.10"
# dependencies = ["fonttools[woff]==4.67.0"]
# ///
"""Build the demo's complete-cluster fallback: uv run scripts/generate_demo_font.py."""

import hashlib
import unicodedata
from pathlib import Path
from urllib.request import urlopen

from fontTools import subset
from fontTools.merge import Merger
from fontTools.otlLib.builder import buildAnchor, buildLookup, buildMarkBasePosSubtable
from fontTools.pens.boundsPen import BoundsPen
from fontTools.ttLib.tables import otTables
from fontTools.ttLib import TTFont

ROOT = Path(__file__).resolve().parents[1]
REVISION = "ffebf8c1ee449e544955a7e813c54f9b73848eac"
BASE_URL = f"https://raw.githubusercontent.com/notofonts/noto-fonts/{REVISION}"
# All regular faces use 1000 units/em. Pin inputs as well as the build tool.
SOURCES = {
    "NotoSerif": "c8f669ceb2c9c60ccf55198b305e08a997ffca79a38cc7eeb551e643cbe66505",
    "NotoSerifDevanagari": "7d4e2b1a7712840218a2726d598490647959ed1104934f3d30a72e77b7c51dad",
    "NotoSerifBengali": "04935aea18655c451d05b1d4bb5bdc835226a221663f89a2ecfbcfdc4483e399",
    "NotoSerifGurmukhi": "e1eaad136a435f6dfa5f0ddd20283b5b8457e04277fd0172170b5972130c05b0",
    "NotoSerifGujarati": "e2f5198b0d222a018cb61f9b1ad367aed0f44106b79e416863f4373df3d4ebe0",
    "NotoSerifOriya": "4bd74637b0714f9bc903b54d5badb623acdd9a0af049de0635f5f499f12351fb",
    "NotoSerifTamil": "ceca1de97e8b2b65c1a7b7b0a62c6015d20c1544ecbfb4b1dd044bf6f56e0cb8",
    "NotoSerifTelugu": "02cf5610b0aa2dc1ab66ed8edadad207ffb86b85eea4889a0b229f041a4b4bf9",
    "NotoSerifKannada": "13621affeebd95198c35c98aaf40027e93c611051490bb48f4766b28f36debd6",
    "NotoSerifMalayalam": "78e06f3779a8bfa022692f8c333bf01c35b6f52de1cf1a07c259122d49f2f1c4",
    "NotoSerifSinhala": "3f5436fa46dd8c31f676021296d448059bea40621aa8a671fcc20beefd812cb0",
}
RANGES = [
    (0x0000, 0x036F), (0x0900, 0x0DFF), (0x1CD0, 0x1CFF),
    (0x1D50, 0x1D51), (0x1DAE, 0x1DAF),  # IPA prenasalization modifiers used by the writer
    (0x1E00, 0x1EFF), (0x2000, 0x209F), (0x20B9, 0x20B9),
    (0x20F0, 0x20F0), (0x25CC, 0x25CC), (0xA789, 0xA789),
    (0xA830, 0xA83F), (0xA8E0, 0xA8FF), (0x11B00, 0x11B0F),
]



# OpenType tags used by the script-specific Noto faces (including legacy tags).
SCRIPT_TAGS = {
    "Devanagari": ["deva", "dev2"], "Bengali": ["beng", "bng2"],
    "Gurmukhi": ["guru", "gur2"], "Gujarati": ["gujr", "gjr2"],
    "Oriya": ["orya", "ory2"], "Tamil": ["taml", "tml2"],
    "Telugu": ["telu", "tel2"], "Kannada": ["knda", "knd2"],
    "Malayalam": ["mlym", "mlm2"], "Sinhala": ["sinh"],
}


def classify_tamil_annotations(font):
    """Let Vedic marks attach past spacing Tamil superscript annotations.

    The merger's locl substitutions select Tamil-specific copies of common
    glyphs. Only classify those copies as GDEF marks: their advances stay intact,
    while Tamil mark-to-base positioning can reach the preceding Tamil base.
    Latin superscripts retain their original classification and shaping.
    """
    gsub = font["GSUB"].table
    cmap = font.getBestCmap()
    annotations = {cmap[ord(char)] for char in "²³⁴"}
    localized = set()
    for record in gsub.ScriptList.ScriptRecord:
        if record.ScriptTag not in SCRIPT_TAGS["Tamil"]:
            continue
        languages = [record.Script.DefaultLangSys] + [lang.LangSys for lang in record.Script.LangSysRecord]
        for language in filter(None, languages):
            for index in language.FeatureIndex:
                feature = gsub.FeatureList.FeatureRecord[index]
                if feature.FeatureTag != "locl":
                    continue
                for lookup in feature.Feature.LookupListIndex:
                    for subtable in gsub.LookupList.Lookup[lookup].SubTable:
                        subtable = getattr(subtable, "ExtSubTable", subtable)
                        for source, target in getattr(subtable, "mapping", {}).items():
                            if source in annotations and target != source:
                                font["GDEF"].table.GlyphClassDef.classDefs[target] = 3
                                localized.add(source)
    if localized != annotations:
        raise ValueError("Missing Tamil-specific superscript glyphs after merging")


def add_shared_mark_positioning(font, source_coverage):
    """Attach borrowed Vedic marks using the receiving glyph's own bounds.

    Merging outlines alone leaves marks at the Devanagari face's origin on
    scripts whose native GPOS tables never included them. Keep native mark
    positioning; supplement only marks missing from each original script face.
    """
    cmap = font.getBestCmap()
    shared = [cp for cp in [0x951, 0x952, *range(0x1CD0, 0x1D00)]
              if cp in cmap and unicodedata.combining(chr(cp)) in (220, 230)]
    glyph_set = font.getGlyphSet()
    bounds = {}
    for name in font.getGlyphOrder():
        pen = BoundsPen(glyph_set)
        glyph_set[name].draw(pen)
        if pen.bounds:
            bounds[name] = pen.bounds
    classes = font["GDEF"].table.GlyphClassDef.classDefs
    # Include shaped conjuncts and ligatures, not just cmap-encoded base letters.
    bases = {}
    for name, (left, bottom, right, top) in bounds.items():
        if classes.get(name) == 3 or font["hmtx"][name][0] == 0:
            continue
        center = round((left + right) / 2)
        bases[name] = {0: buildAnchor(center, round(top + 64)),
                       1: buildAnchor(center, round(bottom - 64))}
    gpos = font["GPOS"].table
    for script, tags in SCRIPT_TAGS.items():
        missing = [cp for cp in shared if cp not in source_coverage[script]]
        if not missing:
            continue
        marks = {}
        for cp in missing:
            name = cmap[cp]
            left, bottom, right, top = bounds[name]
            below = unicodedata.combining(chr(cp)) == 220
            marks[name] = (int(below), buildAnchor(round((left + right) / 2),
                                                   round(top if below else bottom)))
        subtable = buildMarkBasePosSubtable(marks, bases, font.getReverseGlyphMap())
        lookup_index = len(gpos.LookupList.Lookup)
        gpos.LookupList.Lookup.append(buildLookup([subtable]))
        gpos.LookupList.LookupCount = len(gpos.LookupList.Lookup)
        for record in gpos.ScriptList.ScriptRecord:
            if record.ScriptTag not in tags:
                continue
            languages = [record.Script.DefaultLangSys] + [lang.LangSys for lang in record.Script.LangSysRecord]
            for language in filter(None, languages):
                existing = [index for index in language.FeatureIndex
                            if gpos.FeatureList.FeatureRecord[index].FeatureTag == "mark"]
                lookups = [lookup for index in existing
                           for lookup in gpos.FeatureList.FeatureRecord[index].Feature.LookupListIndex]
                feature = otTables.FeatureRecord()
                feature.FeatureTag = "mark"
                feature.Feature = otTables.Feature()
                feature.Feature.FeatureParams = None
                feature.Feature.LookupListIndex = list(dict.fromkeys([*lookups, lookup_index]))
                feature.Feature.LookupCount = len(feature.Feature.LookupListIndex)
                feature_index = len(gpos.FeatureList.FeatureRecord)
                gpos.FeatureList.FeatureRecord.append(feature)
                # Each LangSys must expose a single feature for a given tag;
                # engines can ignore a second "mark" entry instead of combining them.
                language.FeatureIndex = [index for index in language.FeatureIndex if index not in existing]
                language.FeatureIndex.append(feature_index)
                language.FeatureCount = len(language.FeatureIndex)
    # OpenType requires alphabetically ordered feature records; remap references.
    ordered = sorted(enumerate(gpos.FeatureList.FeatureRecord), key=lambda item: item[1].FeatureTag)
    remap = {old: new for new, (old, _) in enumerate(ordered)}
    gpos.FeatureList.FeatureRecord = [record for _, record in ordered]
    gpos.FeatureList.FeatureCount = len(ordered)
    for record in gpos.ScriptList.ScriptRecord:
        languages = [record.Script.DefaultLangSys] + [lang.LangSys for lang in record.Script.LangSysRecord]
        for language in filter(None, languages):
            language.FeatureIndex = sorted(remap[index] for index in language.FeatureIndex)
            if language.ReqFeatureIndex != 0xFFFF:
                language.ReqFeatureIndex = remap[language.ReqFeatureIndex]


def main():
    cache = ROOT / "out/demo-font-sources"
    output = ROOT / "demo/fonts"
    cache.mkdir(parents=True, exist_ok=True)
    output.mkdir(parents=True, exist_ok=True)
    files = []
    copyrights = set()
    source_coverage = {}
    for family, expected_hash in SOURCES.items():
        name = family + "-Regular.ttf"
        path = cache / name
        if not path.exists():
            with urlopen(f"{BASE_URL}/hinted/ttf/{family}/{name}", timeout=60) as response:
                path.write_bytes(response.read())
        if hashlib.sha256(path.read_bytes()).hexdigest() != expected_hash:
            raise ValueError(f"Source checksum mismatch: {path}")
        with TTFont(path) as source:
            source_coverage[family.removeprefix("NotoSerif")] = set(source.getBestCmap())
            copyrights.update(record.toUnicode() for record in source["name"].names if record.nameID == 0)
        files.append(str(path))

    font = Merger().merge(files)
    options = subset.Options()
    options.layout_features = ["*"]
    options.name_IDs = ["*"]
    options.name_languages = ["*"]
    options.name_legacy = True
    subsetter = subset.Subsetter(options=options)
    subsetter.populate(unicodes={code for start, end in RANGES for code in range(start, end + 1)})
    subsetter.subset(font)
    classify_tamil_annotations(font)
    add_shared_mark_positioning(font, source_coverage)
    family = "Inditrans Indic Fallback"
    names = {
        0: "\n".join(sorted(copyrights)), 1: family, 2: "Regular",
        3: "InditransIndicFallback-1.0", 4: family, 6: "InditransIndicFallback-Regular",
        16: family, 17: "Regular",
    }
    # Rename the derivative; retain the original copyright and OFL license records.
    font["name"].names = [record for record in font["name"].names if record.nameID not in names]
    for name_id, value in names.items():
        font["name"].setName(value, name_id, 3, 1, 0x409)
    font.recalcTimestamp = False
    font["head"].modified = 2082844800 + 1674654016
    font.flavor = "woff2"
    destination = output / "inditrans-indic-fallback.woff2"
    font.save(destination)
    with urlopen(f"{BASE_URL}/LICENSE", timeout=60) as response:
        license_text = response.read().decode("utf-8")
    (output / "OFL.txt").write_text("\n".join(sorted(copyrights)) + "\n\n" + license_text, encoding="utf-8")
    print(f"Wrote {destination.relative_to(ROOT)} ({destination.stat().st_size:,} bytes)")


if __name__ == "__main__":
    main()
