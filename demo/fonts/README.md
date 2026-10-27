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
fallback coverage. This font provides both, retaining the source fonts' shaping tables and adding attachment
anchors for borrowed combining marks absent from each original script font.
This also avoids leaving a Telugu or other script's accent at the Devanagari
font's unadjusted position. Native mark positioning is preserved. Diagnostic
font modes can still select a script font, one font, or a custom stack explicitly;
normal mode may use this fallback's serif style for covered Indic characters.

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
rejecting extra dotted circles or character spacing. It also builds the CLI and
shapes forward outputs with five pinned reference fonts for Urdu/Nastaliq, Thai,
Tibetan, Burmese and Khmer. Those downloaded fonts are cached under
`out/demo-font-sources/`; they are not merged into the bundled Indic face.
Checking only that fonts loaded, or checking isolated accent
characters, does not detect the original rendering problem.
