# Inditrans demo

A standalone, buildless page for comparing scripts and fonts, with a script
keyboard and live transliteration. Open `index.html` in a browser, or serve it
from the repository root:

```sh
python3 -m http.server 8000
```

Visit <http://localhost:8000/demo/>. Build matching engine assets with `make wasm`
first. Internet access is required for hosted fonts; engine assets are loaded
from this checkout. Pages deploys the demo and matching checked-in JS/Wasm
bundle together, with the site root redirecting to `demo/`.

The default input is the mixed-script sample, with **Mixed Indic** as the source
and **Indlish** as the output. The **reset icon** restores that text, both default
scripts, and all conversion options. Appearance and keyboard preferences are retained.

Script choices are grouped as **Indic**, **Roman**, and **Perso-Arabic**, with
**Mixed Indic** remaining a separate input choice. Tamil is in Indic and retains
its specialized transliteration rules. Choices marked with **\*** support
accent output in Inditrans; their keyboards offer the corresponding accent keys.
This capability is independent of script family and font coverage.

Conversion options include **Skip-start** and **Skip-end**, both defaulting to
`##`. Text between them is preserved exactly, and the delimiters are removed from
the output. Empty delimiter fields use `##`. Delimiters are saved with the session.

In the normal **Full fallback** mode, a restricted registration of the
[bundled Indic fallback](fonts/README.md) is prioritized for covered Indic
characters, shared Vedic marks, and Tamil superscript annotations. Keeping
these characters in one face lets complete accented syllables render even
when individual script fonts lack a mark. Its Unicode range leaves ordinary
Latin text to the selected font. The unrestricted registration remains later
in the stack for its additional fallback coverage. **Script fonts only**,
**Selected font only**, and **Custom stack** remain diagnostic overrides.
It covers those same scripts in Mixed Indic without depending on device fonts;
Grantha and the other script families use their hosted fonts. Other downloaded
Indic and Roman fonts remain available as further fallbacks. Custom
fonts and **Charis SIL (installed)** must be installed locally. If characters
remain boxes, check that `fonts.googleapis.com` and `fonts.gstatic.com` are
accessible; no Android language or keyboard setting is required for web fonts.

Choose **Find more fonts…** in either font dropdown to open Font Dev Tools (also
available by clicking the version or pressing **Ctrl+Shift+D**). **Find online
fonts** queries the [Fontsource catalog](https://fontsource.org/docs/api/fonts)
for Google Fonts and adds families by script subset; fonts download from Google
only when selected. **Find installed fonts** asks for local-font permission in
supported desktop browsers on HTTPS or localhost. It reads character maps locally
to filter families by basic script coverage; font files are never uploaded.
Coverage does not guarantee shaping quality or Vedic accent support. Other
browsers can still use **Custom installed font…** by family name.

Discovered fonts and dev-tool settings last for the current page session and
survive script changes. Dev tools also support custom fallback stacks and loading
a family from a CSS URL. Failed stylesheet or font downloads report an error.
The loaded-font list reports download status, including idle and failed faces;
use the browser's rendered-font inspector to identify fonts used for particular
glyphs. **Selected font only** removes demo fallbacks, but the browser may still
use system fallback fonts.

The panes, clipboard and saved sessions use the raw engine string. Extended
vowels now use native approximations, so no Telugu separator rearrangement is
needed. Old saved strings remain literal text; the demo does not reconstruct
removed synthetic distinctions.

**Grantha** uses Unicode Grantha characters, `lang="sa-Gran"`, and the hosted
Noto Sans Grantha font. It replaces the former Tamil Extended option.

**Urdu (Nastaliq)** uses `lang="ur"`, RTL direction, additional line spacing, and
Noto Nastaliq Urdu with Noto Naskh Arabic fallback. Its font is downloaded rather
than included in the ten-script Indic fallback. Mixed Indic excludes Urdu;
select Urdu explicitly when converting it. Explicit vowels are reliable inputs;
bare consonants do not acquire guessed vowels. See the
[mapping review](../docs/phonetic-mappings.md) for source limitations.

**Theme** offers System, Light, and Dark. System follows your device’s appearance
and updates automatically when it changes. Your choice is saved in this browser.

Settings and both panes are saved automatically in this browser's local storage.
Reloading restores scripts, fonts, size, options, keyboard mode/layout, group colors,
and text. Turn off **Group colors** in the keyboard controls for uniform keys and
no group color dots in either theme.

The desktop layout can use the full browser width, including wide monitors.
Drag the grip above the keyboard to change its height, or the bottom-right corner
for width and height. Keys scale with the available space; the text panes match
the width and use the remaining height. Focus a grip and use arrow keys to resize,
or double-click it to restore automatic sizing. The size is saved with the session.

Use **↶ / ↷** beside the keyboard mode to undo or redo input edits, including ASCII
typing, on-screen keys, and resetting the input text. Shortcuts: **Ctrl/Cmd+Z** to undo,
**Ctrl/Cmd+Shift+Z** or **Ctrl+Y** to redo. History retains up to 100 edits in the
current page session; reloading starts a fresh history with the saved text.

## Mobile

On screens up to 600px wide and short touch screens in landscape, each pane has
its own script selector in its toolbar. The top bar and both pane toolbars stay
visible while typing with the phone’s keyboard, and both panes keep equal height. Indic sources use
**ASCII → script** automatically with the phone’s English keyboard; Roman sources
accept direct input. Autocorrection and automatic capitalization are disabled
because shortcuts are case-sensitive. The desktop input-mode preference is preserved.

Tap the **book icon** for a full-screen reference matching the selected source, including
vowels, consonants, signs, and symbols. Mixed Indic has a typing-layout selector
in the reference, and typing notes appear after the shortcut tables. Closing the
reference preserves the input selection.
Each pane has a copy control for its complete text. The **reset icon** in the
source toolbar restores the default mixed-script text, source and output scripts,
and conversion options; the conversion-options icon is in the target toolbar.
Desktop uses the same reset behavior.
The text-size slider in the top toolbar adjusts both panes and stays visible while typing.
The **settings icon** opens fonts, theme, and help. All mobile dialogs fill the screen, with scrollable
content and a visible close button. The desktop script keyboard stays hidden on mobile.

## Engine version

The version badge shows `v0.15.0`. The page loads the matching engine assets
shipped with its checkout/site, with the version in asset URLs to refresh browser
caches. Rebuild with `make wasm` after changing mappings or native code. Pages stages `demo/` and the
matching `js/` assets together; deploy both when updating output conventions.

## ASCII input

Choose **ASCII → script** in the keyboard controls. Shortcuts appear below each
key and use the selected Indic input script (or keyboard layout for Mixed Indic).

| Input | Devanagari result | Meaning |
| --- | --- | --- |
| `ki` | कि | Vowels immediately after consonants become signs |
| `k:i` | कइ | `:` forces an independent vowel |
| `` k`Sh `` | क्ष | Backtick (`` ` ``) inserts virama explicitly |
| `kh` / `k;h` | ख / कह | `;` separates overlapping shortcuts |
| `\k` | k | Backslash escapes a literal ASCII character |

Paste and system input methods bypass ASCII conversion. Direct input remains
available; ASCII mode is disabled for Roman input scripts.

## Regression tests

From the repository root, with Node.js installed:

```sh
make test_demo
```

This runs `demo/tests/ascii.test.cjs`, `demo/tests/state.test.cjs`,
`demo/tests/editor.test.cjs`, and `demo/tests/fonts.test.cjs` against the page's
actual code, checking shortcuts,
contextual vowels, escaping, session storage, undo/redo history, glyph spacing,
keyboard resize controls, and font discovery and loading.
It needs no browser, dependencies, or network. It also runs in `make testall` and
the Node.js job in CI for pushes to `main` and pull requests targeting `main`
(documentation-only changes are skipped). It does not test browser interactions.

For actual Chromium DOM paths, after `make wasm` and font verification:

```sh
python3 tests/integration/browser_smoke.py --browser chromium
python3 tests/integration/browser_smoke.py --browser chromium --fixture demo.html
```

The demo fixture checks raw Telugu/Urdu strings, direction/language, dispatched
composition/cut paths, undo/redo and saved-state restoration. Its clipboard-button
check uses a stub; it does not claim OS clipboard or physical IME coverage. It
loads the pinned Nastaliq reference font cached by the font verifier.
