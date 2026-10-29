# Pronunciation wordlists

Each language supplies one UTF-8 file at `wordlists/<script>/wordlist.tsv`.
The bundled Urdu input is `wordlists/urdu/wordlist.tsv`.

## Expected format

Each non-comment row has exactly two tab-separated columns:

```text
# Native<TAB>IPA
کتاب	k ɪ t̪ ɑː b
قلم	q ə l ə m
```

- The first column is one word in the selected native script.
- The second column is one chosen IPA pronunciation.
- Words must be unique after NFC normalization and configured script aliases.
  Duplicate normalized words are compilation errors.
- Use spaces between IPA phones. Keep affricates, aspiration, length and
  nasalization with their phone: `t͡ʃ`, `kʰ`, `ɑː`, `ɛ̃ː`.
- Stress and syllable markers are accepted. Concatenated IPA is also readable,
  but explicit spaces preserve phone boundaries.
- Blank lines and lines starting with `#` are ignored.
- Additional notation, priority or metadata columns are not accepted.

Spelling eligibility comes from the engine's script inventory and optional
settings in `scripts/lexicon_profiles.json`. IPA is mapped to the shared canonical
phoneme inventory. Unsupported spellings/phones are counted in the build report
and omitted from the pack; `--strict` makes them errors. Meaningful written marks
are preserved. Urdu retains its existing restriction to unmarked whole words.

## Compile and verify

```sh
make lexicons
make check-lexicons
make wasm
```

The build produces `lexicons/urdu.iul`, technical build information in
`lexicons/urdu.json`, and `native/src/urdu_embedded_pack.h`. Rebuild native
libraries and CLI executables after changing the embedded dictionary.
`make urdu-lexicon` and `make check-urdu-lexicon` remain compatibility aliases.

The same compiler accepts another source script and one wordlist file:

```sh
printf 'அகம்\tə ɡ ə m\n' > out/tamil.tsv
python3 scripts/build_lexicon.py out/tamil.tsv --script tamil \
  --out out/tamil.iul --report out/tamil.json --strict
./out/inditrans -f tamil -t iast --lexicon out/tamil.iul 'அகம்'
# agam
```

`--check` verifies compiled outputs without writing. The report records the
script, input filename/hash, entry count, rejected rows and output hash.
`embed_lexicon.py --script SCRIPT --source FILE --report REPORT --output HEADER`
emits a C++ header for a compiled dictionary.

## Runtime behavior

The C++ `transliterateWithLexicon(input, from, to, options, pack, output)` and CLI
`--lexicon` accept explicit overrides for any source script. Keys are NFC UTF-8;
use correspondingly normalized input. Unknown words use the ordinary reader.
`DisableInference` and same-script copies retain normal behavior. Currently only
Urdu has a default bundled dictionary. The existing C, Dart and Node Urdu handle
APIs remain compatible wrappers.

Packed keys, pronunciation storage and the reader extension are shared across
scripts. Static policies provide key comparison, spelling restrictions and word
boundaries; Urdu's policy preserves its existing aliases. Other ordinary script
conversions initialize no dictionary. IUL2 stays packed in memory, decoding only
matched units; older IUL1 inputs remain supported. The current packed format
supports up to 256 distinct key code points and the engine's canonical phonemes.
