# Architecture

## Purpose and scope

This document covers the C++23 engine, generated lookup data, and the native/Wasm
boundary. Flutter and Node.js use the same engine and shared test cases.

## System context

```text
Dart / TypeScript caller
    │ native FFI or Wasm export call
    ▼
native/src/inditrans.cpp
    ├─ libinditrans.so / inditrans.dll / platform native libraries
    └─ flutter/assets/inditrans.wasm and js/public/inditrans.js
```

The engine performs no runtime I/O. Reusable script metadata, readers, writers,
equivalents, and Tamil prefixes are immutable compiled data. A transliteration
still allocates its input token vector and output buffer.

## Components

| Component | Files | Responsibility |
|---|---|---|
| C export layer | `native/src/exports.h`, `inditrans.cpp` | `transliterate`, `isScriptSupported`, `releaseBuffer` |
| Generated data | `native/src/script_data.h` | Typed dictionaries, pooled text, token expansions, aliases, writer tables |
| Flat trie | `native/src/static_trie.h` | C++23 `consteval` construction and bounded UTF-8/token-key lookup |
| Script descriptors | `native/src/static_script_types.h`, `static_scripts.h` | Static name resolution, source policies, writer views, Tamil prefix state |
| InputReader | `native/src/inditrans.cpp` | Scan into `vector<TokenOrString>`, then group tokens into `TokenUnit` objects |
| OutputWriter | `native/src/inditrans.cpp` | Apply output rules and append graphemes to `Utf8StringBuilder` |
| UTF-8 helpers | `native/src/utf.h` | UTF-8 encoding/decoding and output-buffer ownership |
| Generator | `tool/generate_headers.dart`, `tool/utils/static_scripts.dart` | Resolve script semantics before C++ compilation |
| Canonical input | `tool/script_data.json`, `tool/reader_data.json` | Script spellings/aliases/equivalents; accents, exclusive symbols, Tamil prefixes |
| Wrappers | `flutter/lib/inditrans.dart`, `nodejs/src/index.ts` | Public APIs, role checks, native/Wasm calls and result release |

## Dependency direction

```text
inditrans.cpp → static_scripts.h → script_data.h → static_script_types.h
                         │                              │
                         └──────────────────────────────┴→ static_trie.h / type_defs.h
inditrans.cpp → script_constants.h / utf.h / inditrans.h
```

`inditrans.cpp` is the engine's translation unit. Headers provide inline code
and constant data; the native engine does not depend on Flutter or Node.js.
Legacy `trie.h`, `char_trie.h`, and `utilities.h` remain utility/test code and
are absent from the production transliteration dependency path.

## Data model

### Script table (`script_data.h`)

Run `dart tool/generate_headers.dart` from the repository root after changing
canonical data. Never edit the generated header or Dart/TypeScript script and
option enums by hand.

The generator preserves base insertion order, first-wins collisions, sorted
equivalent processing, and Latin uppercase/decomposed alternatives. It resolves
textual and `type:index` equivalent targets into complete emitted token sequences,
including the legacy shallow expansion rules. Collisions, partially matched
and unresolved targets are recorded in `out/static-lookup-collisions.json`.
There is no equivalent parser in the runtime engine.

C++ `consteval` builders turn sorted, unique typed entries into exact-sized
flat arrays. Scratch topology and dictionary entries are not addressed by
production runtime data. Checked capacities select 16- or 32-bit trie indices;
other compact fields fail generation/compilation on overflow.

Writer strings share a UTF-8 pool. Each target's descriptor holds eight
compile-time spans of `string_view` entries, an empty `Ignore` slot, its script
type, and Vedic flag. Accent/exclusive-symbol tables and identical class arrays
are shared.

### Token model

```text
TokenType   = Vowel | VowelMark | Consonant | OtherDiacritic | Accent |
              Symbol | VedicSymbol | ExclusiveSymbol | Ignore
ScriptType  = Indic | Tamil | Latin | Others
Token       = { TokenType, idx }
ScriptToken = { TokenType, idx, ScriptType }
TokenUnit   = { leadToken, vowelMark, otherDiacritic, accent }
```

Indices use the shared phoneme ordering. An expansion is a checked 16-bit span:
12 bits for its offset into the token pool, 4 for its count; zero means no match.
`ScriptToken` stays four bytes wide for word-sized copies and preserves the
existing `TokenUnit` field layout. Expansion metadata belongs to the static span,
rather than an extra-token index inside each token.

`TokenOrString` is a `variant<ScriptToken, string_view>`. Its text branch borrows
untransliterated input runs, XML tags, and protected regions. `TokenUnitOrString`
adds the attached diacritics and one-token lookahead used by the writer.

## Transliteration pipeline

1. Resolve source and target names into static descriptors. Canonical names and
   aliases use a compile-time hash-slot table with full ASCII-insensitive equality
   on collisions. There are no reader/writer caches or first-use constructors.
2. `InputReader` chooses explicit, virtual Indic, Roman, or folded Roman policy
   once, then scans bounded UTF-8 input. Matches append their precompiled token
   sequences; `scanUnrecognized` handles raw text, XML, and protected spans as
   borrowed views. It evaluates XML options outside the hot match loop.
3. `InputReader::getNext()` groups tokens. Indic attaches marks and accents;
   Tamil applies superscript and pronunciation rules; Latin groups consonants
   with following vowels/virama.
4. `OutputWriter` looks up target graphemes by token class/index and applies
   target-specific rules. It writes directly to a geometrically growing buffer.
5. The C export releases the result buffer to its caller. The wrapper copies
   the string and calls `releaseBuffer()`.

### Shared non-Roman matcher

Every explicit non-Roman source and virtual `indic` points to one physical
UTF-8 trie. Script identity is a source-mask bit, distinct from `ScriptType`.
Terminals have an explicit primary sequence, rare source variants, and a
precomputed `indic` sequence. Runtime fields are split into state-indexed arrays
so the virtual reader need only touch its sequence array.

Root/high-fanout nodes use direct tables; continuation-only tables use 64
slots. Single-child nodes use equality, small fanout uses linear search, and
larger sparse fanout uses binary search. Two/three-byte prefix accelerators
jump to existing trie states. Up to three unique edges without an intervening
terminal can be compared together, retaining ordinary edges for truncated-input
fallback. Dispatch and compressed paths mark terminal leaves in the index high
bit, allowing selection to skip their node/edge metadata.

Explicit-source prefix masks reject entire unavailable subtrees. A subtree
containing only the requested source can select its primary sequence directly.
All remaining terminal filtering occurs during longest-prefix traversal: a
rejected longer spelling cannot erase an accepted shorter match.

Roman scheme meanings overlap, so those schemes retain separate static
normal/folded dictionaries while using the same flat matcher implementation.
ASCII glyph folding follows the original source-name policy independently
of case-insensitive name/alias resolution (`iso` and `ISO` differ).

### The `indic` virtual script

Its compiled acceptance policy starts with Devanagari including equivalents,
then imports the other non-Roman scripts' base entries in case-insensitive
name order. Other scripts' equivalents are not automatically imported. For
example, Malayalam chillu `ൻ` expands in an explicit Malayalam reader but
is absent from `indic`. Shared spellings retain historical insertion precedence.

### Tamil prefixes

The six canonical prefix strings are tokenized/grouped by the generator.
A static token-key trie replaces the runtime `StatefulTrie` constructor.
Each reader/writer has only a node ID, optional match flag, and length as state.
Normalization of Indic consonants to Tamil and legacy miss/terminal-leaf state
transitions remain in `TamilPrefixLookup`.

## Invariants

- Shared consonant/vowel indices must stay consistent across scripts.
- Latin readers omit vowel marks except virama; writers infer inherent vowels.
- Wrappers restrict `readableLatin`/`wx` to output and `indic` to input.
  Native exact-name role checks retain their legacy behavior (including the
  native `wx` reader); do not infer native restrictions from wrapper roles.
- Native `isScriptSupported` reports physical records, so virtual `indic` is
  absent from that query; wrappers handle the virtual source separately.
- Empty writer entries, the `Ignore` category, and out-of-range indices return empty views.
- Non-Vedic Indic targets automatically request accent stripping.
- Lookup never reads outside `[begin, end)`, including compressed paths and
  truncated UTF-8. Token/variant masks preserve accepted shorter prefixes.
- Data generation and compilation perform all reusable lookup construction.
  Per-call input/output storage remains separate from that requirement.
- Callers release C buffers through `releaseBuffer`; no C++ type crosses the ABI.

### Native ABI and Windows cross-compilation

The C boundary exports only `transliterate`, `isScriptSupported`, and
`releaseBuffer`. MinGW and MSVC clients therefore do not share a C++ ABI.
Windows x86-64 cross-compilation uses `tool/cmake/mingw64.cmake` with C++23 and
MinGW GCC ≥ 13. Static libgcc/libstdc++ linkage avoids their runtime DLL dependencies.

## Script-specific rules

| Feature | Location |
|---|---|
| Tamil allophones and consonant variants | `InputReader::readTamilTokenUnit` |
| Tamil ந→ன position rule, traditional replacements | `OutputWriter::writeTamilTokenUnit` |
| Tamil prefix recognition | Static `TamilPrefixLookup` used by reader/writer |
| Gurmukhi adhak | `InputReader::inferGurmukhiAdhak` |
| Anuswara assimilation | `OutputWriter::inferAnuswara` |
| XML and configured protected spans | `InputReader::scanUnrecognized` |
| Vedic accent stripping | InputReader and OutputWriter |

## Extension points

- Add scripts, aliases, and equivalents to `tool/script_data.json`, then regenerate.
  Add expected outputs to `test-files/test-cases.json` and rebuild distributions.
- Update accents, exclusive symbols, or Tamil prefixes in `tool/reader_data.json`.
- Add option metadata to `tool/options.json`, regenerate, and implement its behavior
  in the engine. Regenerate FFI bindings when the export header changes.
- Change matcher layout in `static_trie.h`; preserve bounded matching and the
  source policies. Compare native and Wasm performance and size.

## Related documents

- [Agent guide](AGENTS.md)
- [Static lookup design and measurements](docs/static-lookup-design.md)
- [Flutter usage](flutter/README.md)
- [Node.js usage](nodejs/README.md)
- [C exports](native/src/exports.h)
