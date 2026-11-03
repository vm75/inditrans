# Indic fallback font

`inditrans-indic-fallback.woff2` is a renamed, merged and subsetted derivative of
Noto Serif Regular and its Devanagari, Bengali, Gurmukhi, Gujarati, Oriya, Tamil,
Telugu, Kannada, Malayalam and Sinhala families. It is distributed under the
[SIL Open Font License](OFL.txt); copyright notices are included in the font
and license file. The subset includes the four IPA prenasalization modifiers
used by the engine, so Roman output also has complete glyph coverage.

Browsers can keep a base letter and its combining marks in one font run.
A fallback list of separate script and accent fonts therefore still produces
missing-glyph boxes when no single face supports the whole cluster. Normal
demo mode prioritizes a restricted family registration of this face across its
covered Indic scripts, shared Vedic marks, and Tamil superscript annotation
digits. Unicode-range restrictions leave ordinary Latin text with the selected
font. The original unrestricted family remains later in the stack for extra
fallback coverage. This font provides both, retaining the source fonts' shaping
tables and adding attachment anchors for borrowed combining marks absent from
each original script font.
This also avoids leaving a Telugu or other script's accent at the Devanagari
font's unadjusted position. Most native mark positioning is preserved; the
generator splits problematic Vedic mark classes for independent vowels and
repairs those specific anchors without changing other diacritics. Diagnostic
font modes can still select a script font, one font, or a custom stack explicitly;
normal mode may use this fallback's serif style for covered Indic characters.

Tamil's localized superscript glyphs (`²³⁴`) are classified as marks in
GDEF. This lets an accent after a superscript attach to the preceding Tamil base
instead of floating after the digit. The digit keeps its original advance, and
Latin superscripts retain their original glyph classes. Text and clipboard
contents remain the literal engine output.

Sources are pinned to the archived `notofonts/noto-fonts` revision
[`ffebf8c`](https://github.com/notofonts/noto-fonts/tree/ffebf8c1ee449e544955a7e813c54f9b73848eac).
The generator records and verifies each input's SHA-256 hash. Regenerate from the
repository root with the isolated, pinned dependencies in the script:

```sh
uv run scripts/generate_demo_font.py
```

Downloads are cached in `out/demo-font-sources/`. Building the demo does not
require Python, uv, fontTools, or font generation; the WOFF2 is checked in and
served with the page.

Validate the checked-in font against the demo's current keyboard data and
complete conjunct/vowel/accent combinations:

```sh
uv run scripts/verify_demo_font.py
```

This uses HarfBuzz shaping and fails if any result contains glyph ID zero
(`.notdef`), and checks that borrowed Vedic marks attach without adding spacing.
It checks raw Telugu vowels/conjuncts with real accents against native clusters,
rejecting extra dotted circles or character spacing. It checks Tamil superscripts
against unannotated syllables: accent glyph positions must match, with no inserted
dotted circles or extra accent spacing. It also measures actual HarfBuzz-positioned
accent ink bounds for all ten Indic font families, and checks the resulting
typographic line metrics against the pinned source fonts. It also builds the CLI and shapes forward
outputs with five pinned reference fonts for Urdu/Nastaliq, Thai,
Tibetan, Burmese and Khmer. Those downloaded fonts are cached under
`out/demo-font-sources/`; they are not merged into the bundled Indic face.
Checking only that fonts loaded, or checking isolated accent
characters, does not detect the original rendering problem.

## Vedic positioning: measurements and cause

The font has **four independent vertical concepts**, which must not be conflated:

- `head.yMin/yMax` are true outline extrema, not attachment coordinates or
  normal line-height requirements. The combined font retains `[-710, 1397]`.
- `hhea` ascent/descent and `OS/2.sTypo*` describe default typographic line
  boxes. The merger originally took the maximum ascent from one source and
  the minimum descent from a different source, creating a line box taller than
  *any one source font*. The generator now uses the tallest **complete**
  original source line box, retaining the highest source ascent.
- `OS/2.usWinAscent/Descent` are conservative clipping metrics. They remain
  `1092/710`; shrinking them to match a normal line box would risk clipping
  unusual combining clusters.
- GPOS `abvm`/`blwm` MarkBase anchors position native U+0951/U+0952; the
  supplemental `mark` lookups position borrowed accents absent from the
  original font. The original Vedic offsets are **not** caused entirely by
  the generated bounding-box anchors.

Font-wide metrics, in units per 1,000-unit em:

| Metric | Original combined | Repaired combined |
| --- | ---: | ---: |
| `head.yMin/yMax` | -710 / 1397 | -710 / 1397 |
| `hhea` ascent/descent | 1092 / -710 | 1092 / -528 |
| `OS/2` typographic ascent/descent | 1092 / -710 | 1092 / -528 |
| Default typographic height | **1802** | **1620** |
| `OS/2` Win ascent/descent | 1092 / 710 | 1092 / 710 |

Kannada has the largest original Noto line box, 910 - (-710) = 1620
units. The merged font's new 1620-unit line box matches that largest
whole-source height rather than adding independently selected extremes.
This reduces default line-box height by ~10.1%; it **does not** change
glyph outlines or guarantee that all tall conjuncts fit in a fixed line.

In the pinned Noto fonts, `abvm` and `blwm` often attach U+0951/U+0952 at
hundreds of font units from independent vowels. These offsets survive
merging unchanged. Noto Devanagari, Gujarati and Gurmukhi already have
reasonable simple-glyph gaps, and Oriya/Sinhala are supplied by the
generator's supplemental anchors. Their existing placements are retained.

For five other scripts, the generator keeps the native MarkBase lookup and
**splits U+0951/U+0952 into new mark classes** inside that lookup. It
copies every original base anchor, changing only entries for independent
vowels with measured excessive clearance. This prevents moving nasal
marks, other diacritics, ligatures, and native mark-to-mark chains. It uses
the actual localized mark glyph selected by HarfBuzz, native anchor data,
and ordinary vowel ink bounds—not the global head bounding box.
The target clearances (64 units above, 76 below) are calibrated against
the already-correct Noto script references. Devanagari and similar
correct placements are not adjusted.

### Before and after: vertically adjacent ink

The table reports the measured vertical distance from the independent
vowel's ink to the visible Vedic mark, using **fully shaped outlines** and
the font's own 1000-unit em. The native fonts' simple-vowel positions are
the unmodified baseline for U+0951/U+0952 wherever those codepoints exist.

| Script, independent vowel | U+0951 before → after | U+0952 before → after |
| --- | ---: | ---: |
| Devanagari `अ` | 54 → 54 | 71 → 71 |
| Bengali `অ` | 334 → 64 | 449 → 76 |
| Gurmukhi `ਅ` | 57 → 57 | 90 → 90 |
| Gujarati `અ` | 69 → 69 | 72 → 72 |
| Oriya `ଅ` | 64 → 64 | 64 → 64 |
| Tamil `அ` | 353 → 64 | 281 → 76 |
| Telugu `అ` | 394 → 64 | 570 → 76 |
| Kannada `ಅ` | 321 → 64 | 530 → 76 |
| Malayalam `അ` | 307 → 64 | 402 → 76 |
| Sinhala `අ` | 64 → 64 | 64 → 64 |

A negative ink-gap measurement for a split vowel or conjunct is not,
by itself, a glyph collision: those clusters can contain multiple disjoint
marks at different horizontal coordinates. The verifier therefore enforces
clear, non-overlapping ink for ordinary independent vowels, and separately
checks complete cluster shaping, GDEF nonspacing behavior, and Tamil
superscript attachment. HarfBuzz alone cannot validate browser fallback
or CSS line-box behavior.

### Reproduce and visually inspect

```sh
uv run scripts/generate_demo_font.py
uv run scripts/measure_vedic_font.py
uv run scripts/verify_demo_font.py
python3 -m http.server 8000
```

Open [the font comparison](vedic-accent-comparison.html) on the local
server. It shows the hash-pinned native Noto sources next to the bundled
face for each script, with an adjustable line-height. Native TTF files
are read from the generator's `out/demo-font-sources/` cache.
CI also verifies deterministic WOFF2 regeneration and uploads a
Chromium screenshot of the comparison as a build artifact.

### Remaining constraints

The native Noto fonts often set a high accent anchor on consonants in
order to clear dependent vowels, and a low anchor to clear below-base
components. A single OpenType MarkBase anchor on a consonant cannot
simultaneously optimize the unsuffixed consonant and every dependent
vowel/conjunct context. This targeted change deliberately **does not
move** those consonant anchors: some plain consonants therefore still
show the source font's larger gap (e.g., unmodified Tamil `க॑` and
Telugu `క॒`). A complete correction of those contexts would need
carefully tested contextual GPOS positioning, rather than a blanket
script-wide offset. Genuine tall clusters and stacked Vedic marks may
still need extra application line spacing; this font does not attempt to
compress their outlines.
