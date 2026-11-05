# Indic fallback font

Each tagged GitHub Release provides `inditrans-fonts-vX.Y.Z.zip`, containing
both fonts, `OFL.txt`, and the browser/Flutter instructions below. Review tools
and development documentation remain in the repository.

## Use in client applications

The matching [TTF](inditrans-indic-fallback.ttf) and
[WOFF2](inditrans-indic-fallback.woff2) files contain the same font, including
Indic shaping and Vedic accent positioning. Use WOFF2 for smaller browser
downloads and TTF for Flutter or desktop applications. Copy [OFL.txt](OFL.txt)
with either file. No pane library or separate CSS file is required.

### Browser

Copy the font beside your page and include these rules in your own stylesheet:

```html
<style>
  @font-face {
    font-family: 'Inditrans Indic Fallback';
    src: url('./inditrans-indic-fallback.woff2') format('woff2');
    font-style: normal;
    font-weight: 400;
    font-display: swap;
  }
  .inditrans-text {
    font-family: 'Inditrans Indic Fallback', serif;
    font-size: 1.25rem;
    line-height: 1.8;
    white-space: pre-wrap;
  }
</style>

<textarea class="inditrans-text" lang="sa-Deva"
  aria-label="Editable Sanskrit text">नमस्ते</textarea>
<div class="inditrans-text" lang="sa-Deva">नमस्ते</div>
```

The line height is a starting point; allow enough vertical space for accents
and avoid fixed heights that clip text. Use `.value` for textarea output and
`.textContent` for a text display when connecting the existing Inditrans API.
TTF can also be used in `@font-face` with `format('truetype')`.

### Flutter

Copy the TTF to your application's `assets/fonts/` directory and declare it
in the application's `pubspec.yaml`:

```yaml
flutter:
  fonts:
    - family: InditransIndic
      fonts:
        - asset: assets/fonts/inditrans-indic-fallback.ttf
          weight: 400
```

Apply the font to native editable and read-only widgets:

```dart
const indicStyle = TextStyle(fontFamily: 'InditransIndic', fontSize: 20);

// Editable; create and dispose the controller in the owning widget.
TextField(controller: controller, style: indicStyle, maxLines: null);

// Read-only and selectable.
SelectableText(output, style: indicStyle);
```

CSS is not used by Flutter. Use the existing Inditrans Dart API to compute
`output`; the font only controls rendering and does not add ASCII typing or
transliteration behavior. The TTF is provided here for copying, not bundled
in the Flutter package. Check accent placement and clipping on your target
Flutter platforms; browser shaping checks do not validate Flutter rendering.

These fonts cover Devanagari, Bengali (including Assamese), Gurmukhi, Gujarati,
Odia, Tamil, Telugu, Kannada, Malayalam and Sinhala, plus Roman/IPA coverage.
They do not cover every
script supported by the engine. Urdu, Grantha, Thai, Tibetan, Burmese and Khmer
need other fonts; Urdu also needs RTL direction in the client UI.

## Sources and generation

`inditrans-indic-fallback.ttf` and `.woff2` are renamed, merged and subsetted derivatives of
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
generator adds measured contextual adjustments for the three reviewed accents
without changing the positions of other diacritics. Diagnostic
font modes can still select a script font, one font, or a custom stack explicitly;
normal mode may use this fallback's serif style for covered Indic characters.

Tamil's localized superscript glyphs (`²³⁴`) are classified as marks in
GDEF. This lets an accent after a superscript attach to the preceding Tamil base
instead of floating after the digit. The digit keeps its original advance, and
Latin superscripts retain their original glyph classes. Text and clipboard
contents remain the literal engine output.

Sources are pinned to the archived `notofonts/noto-fonts` revision
[`ffebf8c`](https://github.com/notofonts/noto-fonts/tree/ffebf8c1ee449e544955a7e813c54f9b73848eac).
The generator records and verifies each input's SHA-256 hash and pins
both `head.created` and `head.modified` timestamps. FontTools Merger
otherwise sets `head.created` to the build time, which changes the
whole-font checksum and font bytes even when every other table is identical.
Regenerate from the
repository root with the isolated, pinned dependencies in the script:

```sh
uv run scripts/generate_demo_font.py
```

Font downloads and the hash-pinned source license are cached in
`out/demo-font-sources/`, allowing repeat builds without network access. Building the demo does not
require Python, uv, fontTools, or font generation; both formats are checked in.
The demo prefers WOFF2 with TTF as a fallback source for the same face.

Validate the checked-in font against the demo's current keyboard data and
complete conjunct/vowel/accent combinations:

```sh
uv run scripts/verify_demo_font.py
```

This checks matching TTF/WOFF2 character coverage and glyph positioning for
the shared shaping cases. It uses HarfBuzz shaping and fails if any result contains glyph ID zero
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
generator's supplemental anchors. Healthy native placements are retained;
outlying vowels and composites are handled by the contextual pass.

The generator retains native attachment tables, then adds contextual corrections
for U+0951, U+0952, and U+1CDA across the ten bundled script families. It measures
fully shaped glyphs from the canonical script data, including vowels, consonants,
all dependent signs and diacritics, and representative conjunct structures.
Pronunciation annotations and unrelated preceding syllables do not determine
the accent's clearance. Reordered glyphs are identified through localization
and shaping differences rather than assuming that the accent is last.

The target clearances are 64 units above and 76 below, calibrated against
healthy native Noto examples. Healthy placements are retained; collisions and
excessive clearances are adjusted. Bengali pre-base i-matra plus double svarita
also receives horizontal correction derived from the native single-stroke
reference and the difference in stroke widths. Its final position is checked
against repha and other overlapping ink.

### Before and after: vertically adjacent ink

The table reports the measured vertical distance from the independent
vowel's ink to the visible Vedic mark, using **fully shaped outlines** and
the font's own 1000-unit em. The native fonts' simple-vowel positions are
the unmodified baseline for U+0951/U+0952 wherever those codepoints exist.

| Script, independent vowel | U+0951 before → after | U+0952 before → after |
| --- | ---: | ---: |
| Devanagari `अ` | 54 → 54 | 71 → 71 |
| Bengali `অ` | 334 → 62 | 449 → 77 |
| Gurmukhi `ਅ` | 57 → 57 | 90 → 90 |
| Gujarati `અ` | 69 → 69 | 72 → 72 |
| Oriya `ଅ` | 64 → 64 | 64 → 64 |
| Tamil `அ` | 353 → 65 | 281 → 77 |
| Telugu `అ` | 394 → 66 | 570 → 74 |
| Kannada `ಅ` | 321 → 65 | 530 → 74 |
| Malayalam `അ` | 307 → 63 | 402 → 74 |
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
make test_fonts
python3 -m http.server 8000
```

Open [the font comparison](vedic-accent-comparison.html) on the local
server. It shows the hash-pinned native Noto sources next to the bundled
face for each script, with an adjustable line-height. Native TTF files
are read from the generator's `out/demo-font-sources/` cache.
CI also verifies deterministic TTF and WOFF2 regeneration and uploads a
Chromium screenshot of the comparison as a build artifact.

### Contextual positioning and regression coverage

`scripts/refine_vedic_anchors.py` measures 110,145 clusters and builds a single
final contextual GPOS lookup. Longer contexts take priority over shorter base
patterns. Explicit zero-shift guards protect combinations that already have
adequate clearance, including scripts that reorder a vowel sign after the
accent. Rule compression compares both backtrack and lookahead contexts, and
shared shift lookups plus extension subtables keep OpenType offsets in range.

The pinned build uses 24,682 context rules, 220 shared shift lookups, and 63
context subtables. Its WOFF2 is 501,584 bytes, compared with 446,472 bytes for
the preceding sparse repair (+55,112 bytes, approximately 12%). Each generated
context is reshaped during the build: only the accent may move, and character
advances must stay identical. The outline measurement helper flattens curves
to half-unit accuracy and samples ink envelopes every four font units. This
distinguishes real collisions from overlapping boxes with disjoint ink.

The verifier checks the saved, decoded WOFF2 against all 110,145 placements,
the 2,560 incomplete human reviews in `test-files/vedic-font-review.json`,
88 accepted controls, and 50 horizontal reports. It also retains the 83
representative samples, 378 original contextual checks, 105 Tamil annotation
cases, and the complete glyph/shaping corpus. Missing review entries and null
axes remain unreviewed; a green geometry check is not a new human approval.

Tamil spacing modifiers `ˮ` and `꞉` get localized mark classes, retaining their
widths. A final GSUB context removes a spurious dotted circle before the Vedic
mark and lets it attach to the preceding Tamil base. Trailing aytham is handled
only in that context; normal aytham-before-consonant spelling is preserved.

For native browser display, editing, selection, and spacing checks:

```sh
python3 tests/integration/browser_smoke.py --fixture vedic-font.html
```

### Remaining constraints

Arbitrary stacked conjuncts and stacked Vedic accents outside the measured
corpus can still require further review and application line spacing. Unicode
script itemization can also create a boundary that OpenType cannot cross:
U+02BC (`ʼ`) has script extensions excluding Tamil, so Chrome can split the
`ம்ʼ` spelling from its accent. The reviewed U+02EE (`ˮ`) spelling stays in
the Tamil run. The font supports U+02BC when explicitly shaped as Tamil, but
font positioning alone cannot bridge separate browser shaping runs; the engine
output is not rewritten to hide this distinction. See the Unicode
[ScriptExtensions data](https://www.unicode.org/Public/17.0.0/ucd/ScriptExtensions.txt).

Some review labels also differ for identical upper ink, such as `টৃ॑` versus
`টৄ॑`. The verifier protects consistent typography instead of encoding a
different upper-accent position solely because the lower vowel sign changed.
Use the browser comparison and visual review for optical judgments beyond the
measured coverage.

## Visual failure review

Open [the visual review page](vedic-review.html) through the local server to
review the same bundled font without modifying it. Select a script and corpus
group, then click each accented sample to choose **Too close**, **Too far away**,
or **Okay** for vertical spacing, and **Too far left**, **Okay**, or **Too far
right** independently for horizontal placement. Either axis can be marked
**Unsure**, and an optional note saves automatically. All cells stay white. Colored markers show non-okay ratings: red − for
too close, amber + for too far, blue ← for too far left, purple → for too far
right, and gray ? for unsure. Vertical markers sit at the top left and
horizontal markers at the top right. Okay ratings have no marker. Dashed borders indicate an unreviewed
or unsure axis; a solid border with no markers means both axes are okay. Exact code points, font size,
browser, and font fingerprint are included in **Export reviews (JSON)**. Reviews and custom
lines persist in local storage; export regularly for a portable copy.

The scrollable corpus uses the demo keyboard tables for every Indic script
declaring Vedic accent support, filtered to its native keyboard inventory
(including Tamil superscripted consonants, with vowel signs before superscripts):
distinct vowels/consonants, consonants with
vowel signs, vowels/consonants with diacritics, all ordered two-consonant
combinations, and those combinations with vowel signs. This is a mechanical
shaping corpus, not a claim that every combination is linguistically valid or
that every possible conjunct is covered. Add longer combinations or actual
words as custom lines. Unaccented lines receive all three accents; lines
already containing an accent retain its exact position. A native text-area
preview is available through **Edit selected** after selecting a sample.
Controls stay pinned at the top; About, Custom, and Edit selected
open dialogs so the grid contains only the accented characters.

The page blocks review if the bundled font fails to load. It needs no engine
build or external font downloads. Unreviewed samples are never counted as
correct. Reopening the page restores reviews, but exported JSON is not an
import format in this first version.

Use arrow keys on a focused sample to navigate through the continuous grid;
Home/End move to the first/last sample and Page Up/Down move by a screenful. Rate directly with
**1 = too close**, **2 = okay**, **3 = too far away** for vertical spacing;
**4 = too far left**, **5 = okay**, **6 = too far right** for horizontal placement.
Press **E** to edit. The review dialog stays open so both axes can be rated;
close it with Done or Escape.
The editor can save changes as a custom case without modifying the original.
Shortcuts do not intercept text entry. **All combinations** combines and
deduplicates the generated groups and custom lines (all shown in one scrollable grid). Script,
group, accent, filter, font size and selected sample persist with reviews
in browser local storage.

JSON schema version 2 exports separate `vertical` and `horizontal` fields plus
`note`. An unreviewed axis is `null`; uncertainty is `unsure`. Existing stored
vertical reviews migrate automatically without assuming horizontal placement
is okay. The Problems filter includes an issue on either axis, Reviewed
includes any explicit rating or note, and Incomplete / unsure includes cases
with either axis missing or unsure. Horizontal review records the observed
misplacement, not a requested direction to shift the glyph.

The review grid uses fixed character columns with separate rows for each
accent, vowel sign, diacritic, and conjunct prefix. Consonants + vowel signs
also shows the bare-consonant rows for comparison. Blank slots preserve column
alignment when filtering or removing duplicate stems. The grid scrolls in both
directions; row headings stay visible on the left.

Shift-click or Shift+arrows selects a rectangle, Ctrl/Cmd-click toggles a cell,
and a row heading selects its entire row (Ctrl/Cmd adds it to the selection).
Select all or Ctrl/Cmd+A selects the displayed corpus; Escape clears selection.
The 1–6 rating shortcuts and dialog choices apply to every selected cell while
preserving the other axis. Enter opens the dialog for the selection; editing
still targets the focused cell. Notes typed in the dialog replace notes on all
selected cells. Selection is saved as compact index ranges with the view and
restored on reload. Each cell still exports its own exact text and ratings.
