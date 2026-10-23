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
uses a fixed input token window on the stack and allocates result storage. The C API
writes to a growable UTF-8 buffer; the C++ string API writes directly to its
returned `std::string`.

## Components

| Component | Files | Responsibility |
|---|---|---|
| C export layer | `native/src/exports.h`, `inditrans.cpp` | `transliterate`, `isScriptSupported`, `releaseBuffer` |
| Generated data | `native/src/script_data.h` | Typed dictionaries, pooled text, token expansions, aliases, writer tables |
| Trie builders | `native/src/static_trie.h`, `packed_trie.h` | C++23 `consteval` topology, packing, and bounded byte/token-key lookup |
| Script descriptors | `native/src/static_script_types.h`, `static_scripts.h` | Static name resolution, source policies, writer views, Tamil prefix state |
| Token Stream Model | `native/src/token_stream.h` | `TokenOrString` discriminated union, `TokenUnitOrString`, zero-allocation streaming adaptors |
| Preservation Policy | `native/src/preservation_policy.h` | XML/HTML tag preservation, protected skip spans, raw run scanning, special marker filtering |
| Contextual Handlers | `native/src/contextual_handlers.h` | Anuswara phoneme assimilation, ASCII numeral formatting, Tamil traditional replacements |
| Script Handlers | `gurmukhi_handler.h`, `tamil_handler.h`, `indic_handler.h`, `latin_handler.h` | Isolated, independently testable reading and writing rules for each script family |
| Engine Pipeline | `native/src/inditrans.cpp` | Coordinates input tokenization, streaming token units, lookahead, and output writing via static handler dispatch |
| UTF-8 helpers | `native/src/utf.h` | UTF-8 encoding/decoding and output-buffer ownership |
| Generator | `tool/generate_headers.py`, `tool/python/static_scripts.py` | Resolve script semantics before C++ compilation |
| Canonical input | `tool/script_data.json`, `tool/reader_data.json` | Script spellings/aliases/equivalents; accents, exclusive symbols, Tamil prefixes |
| Wrappers | `flutter/lib/inditrans.dart`, `nodejs/src/index.ts` | Public APIs, role checks, native/Wasm calls and result release |

## Dependency direction

```text
inditrans.cpp → {preservation_policy.h, contextual_handlers.h, gurmukhi_handler.h,
                 tamil_handler.h, indic_handler.h, latin_handler.h}
                 │
                 └─→ token_stream.h → static_scripts.h → script_data.h → static_script_types.h
                                              │                              │
                                              └──────────────────────────────┴→ packed_trie.h → static_trie.h / type_defs.h
inditrans.cpp → script_constants.h / utf.h / inditrans.h
```

`inditrans.cpp` coordinates the transliteration pipeline without embedding language-specific rules. Headers provide inline code, static handlers, and constant data; the native engine does not depend on Flutter or Node.js.
Legacy `trie.h`, `char_trie.h`, and `utilities.h` remain utility/test code and
are absent from the production transliteration dependency path.

## Data model

### Script table (`script_data.h`)

Run `python3 tool/generate_headers.py` from the repository root after changing
canonical data. Never edit the generated header or Dart/TypeScript script and
option enums by hand.

The generator preserves base insertion order, first-wins collisions, sorted
equivalent processing, and Latin uppercase/decomposed alternatives. It resolves
textual and `type:index` equivalent targets into complete emitted token sequences,
including the legacy shallow expansion rules. Collisions, partially matched
and unresolved targets are recorded in `out/static-lookup-collisions.json`.
There is no equivalent parser in the runtime engine.

Readable `SourceEntry` records associate UTF-8 spellings with source masks,
named token sequences, virtual results, and exceptional variant ranges.
`sequenceId` resolves canonical sequence boundaries at compile time;
`writerChar<writerText>` checks literal bytes against pooled offsets. These
source records, sequence ranges, and canonical lookup tables are compile-time
inputs and disappear from the production binary.

C++ `consteval` builders form a full scratch byte topology, then retain only
reachable branching states and accepted prefixes with children. Leaves encode
terminal IDs directly in dispatch words. Sparse edges, compressed paths, and
pooled dispatch pages reference compact states. Checked capacities select 16-
or 32-bit trie indices; other fields fail generation/compilation on overflow.
The Tamil token-key trie retains the generic flat representation.

Writer strings share a UTF-8 pool, including identical substrings and overlapping
suffixes. The generator assigns offsets; `writerChar` checks every complete
glyph against the resulting bytes at compile time. Lookup still uses one
direct offset and length. Each target's descriptor holds eight
compile-time spans of compact offset/length entries, an empty `Ignore` slot, its script
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

`TokenOrString` stores a pointer and a word. A null pointer identifies a four-byte
token payload; otherwise the word is a borrowed text length. It occupies 16 bytes
on native x86-64 and 8 bytes on Wasm32, without a separate variant discriminator.
Text borrows untransliterated input runs, XML tags, and protected regions.
`TokenUnitOrString` remains a variant and adds the attached diacritics and
one-token lookahead used by the writer.

## Transliteration pipeline

1. Resolve source and target names into static descriptors. Canonical names and
   aliases use a compile-time hash-slot table with full ASCII-insensitive equality
   on collisions. There are no reader/writer caches or first-use constructors.
2. `InputReader` chooses explicit, virtual Indic, Roman, or folded Roman policy
   once, then scans bounded UTF-8 input on demand. Matches feed precompiled token
   sequences into a fixed 16-entry window. Refills reuse the window after the
   current sequence is consumed, with no allocation, shifting, or ring arithmetic.
   The checked four-bit sequence count bounds expansions at 15 tokens; live
   reader calls require one token of lookahead. `PreservedSpanHandler::pullUnrecognized`
   handles raw text, XML, and protected spans as borrowed views. XML options are evaluated
   outside the hot match loop.
3. `InputReader::getNext()` coordinates token assembly by delegating to isolated
   reader handlers: `IndicReaderHandler` attaches marks and accents, `TamilReaderHandler`
   applies superscript parsing and contextual pronunciation rules, `GurmukhiAdhakHandler`
   evaluates gemination without consumption, and `LatinReaderHandler` groups consonants
   with following vowels or synthesizes virama. Handlers are `friend` classes to
   `InputReader`, eliminating abstraction overhead by accessing the token buffer directly.
4. One dispatch selects an Indic, Tamil, or Latin `OutputWriter` specialization.
   The writer delegates to `IndicWriterHandler`, `TamilWriterHandler`, or `LatinWriterHandler`
   via compile-time `if constexpr` branch elimination. Handlers receive a reference
   to the strongly-typed `Writer` rather than using `std::function` or lambda callbacks,
   allowing the compiler to fully inline output emission. Inactive handlers occupy no
   state or execution time (`[[no_unique_address]]`). Both sinks write directly to
   result storage. `SpecialMarkerFilter` copies retained text runs together and checks
   ASCII before searching the non-ASCII marker string. Roman-to-non-Latin output
   reserves three input bytes per byte as a growth hint; both sinks can still grow
   for larger expansions.
5. The C export releases the result buffer to its caller. The wrapper copies
   the string and calls `releaseBuffer()`.

### Shared non-Roman matcher

Every explicit non-Roman source and virtual `indic` points to one physical
UTF-8 trie. Script identity is a source-mask bit, distinct from `ScriptType`.
Terminals have an explicit primary sequence, rare source variants, and a
precomputed `indic` sequence. Leaves select terminal-indexed fields; branching
states retain parallel fields indexed by compact state, avoiding a dependent
node-to-terminal load. The virtual reader touches only its sequence field. Empty alternative offsets
are canonical zero at compile time; begin/count retain their original widths
and direct runtime access. Parallel direct fields keep traversal loads
independent. Keep lookup representation changes evidence-driven and verify
correctness, runtime performance, memory, and artifact size with the procedures
in [the performance guide](docs/performance.md).

Root/high-fanout nodes use direct tables; continuation-only tables use 64
slots. Single-child nodes use equality, small fanout uses linear search, and
larger sparse fanout uses binary search. The compiler chooses whether to inline
the transition helper. Two/three-byte
prefix accelerators jump to compact trie states. Equivalent 64-slot dispatch
pages share pooled storage. Up to three unique edges without an intervening
terminal can be compared together; a truncated or mismatched compressed path
stops while preserving the last accepted match. Bypassed nonterminal states and
fallback edges are absent. Path payloads are indexed directly by compact state
so they can load alongside node metadata. The index high bit marks terminal
leaves, which have no node/edge record.

Explicit-source prefix masks reject entire unavailable subtrees. A subtree
containing only the requested source can select its primary sequence directly.
All remaining terminal filtering occurs during longest-prefix traversal: a
rejected longer spelling cannot erase an accepted shorter match.

Roman scheme meanings overlap, so those schemes retain separate static
normal/folded dictionaries while using the same packed matcher implementation.
ASCII glyph folding follows the original source-name policy independently
of case-insensitive name/alias resolution (`iso` and `ISO` differ).

### The `indic` virtual script

Its compiled acceptance policy starts with Devanagari including equivalents,
then imports the other non-Roman scripts' base entries in case-insensitive
name order. Other scripts' equivalents are not automatically imported. For
example, Malayalam chillu `ൻ` expands in an explicit Malayalam reader but
is absent from `indic`. Shared spellings retain historical insertion precedence.

### Tamil prefixes

The six canonical prefix strings are emitted by the generator and
tokenized/grouped by a `consteval` builder using the compiled reader data.
A static token-key trie replaces the runtime `StatefulTrie` constructor.
Each reader/writer has only a node ID, optional match flag, and length as state.
Normalization of Indic consonants to Tamil and legacy miss/terminal-leaf state
transitions remain in `TamilPrefixLookup`.

## Invariants

- Shared consonant/vowel indices must stay consistent across scripts.
- Latin readers omit vowel marks except virama; writers infer inherent vowels.
- Wrappers restrict `inglish`/`wx` to output (write-only) and `indic` to input (read-only).
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

## Script-specific rules and isolated handlers

| Feature | Handler / Location | Responsibility |
|---|---|---|
| Tamil allophones and consonant variants | `handlers::TamilReaderHandler` (`native/src/tamil_handler.h`) | Parses superscript/subscript numbers; evaluates context-dependent voicing and ச→ஸ alternation |
| Tamil ந→ன position rule, traditional replacements | `handlers::TamilWriterHandler` (`native/src/tamil_handler.h`) | Enforces dental vs alveolar nasal placement, traditional Grantha substitutions, superscript reordering |
| Tamil prefix recognition | Static `TamilPrefixLookup` used by `TamilReaderHandler` & `TamilWriterHandler` | Tracks prefix trie transitions and maintains start-of-stem pronunciation rules |
| Gurmukhi adhak | `handlers::GurmukhiAdhakHandler` (`native/src/gurmukhi_handler.h`) | Evaluates adhak gemination symbol, de-aspirates consonant, emits virama unit |
| Anuswara assimilation | `handlers::AnuswaraAssimilator` (`native/src/contextual_handlers.h`) | Assimilates anuswara to class nasal across consonant vargas; shared between Tamil and Latin writers |
| ASCII numeral conversion | `handlers::AsciiNumeralFormatter` (`native/src/contextual_handlers.h`) | Formats script numerals as ASCII digits '0'..'9' when `ASCIINumerals` is enabled |
| XML and configured protected spans | `handlers::PreservedSpanHandler` (`native/src/preservation_policy.h`) | Streams through XML tags, configured skip delimiters (`skipStart`/`skipEnd`), and unmapped byte runs |
| Vedic accent stripping & special marker filtering | `handlers::SpecialMarkerFilter` (`native/src/preservation_policy.h`) | Filters non-ASCII disambiguation markers unless `RetainSpecialMarkers` is set |

## Extension points

- Add scripts, aliases, and equivalents to `tool/script_data.json`, then regenerate.
  Add expected outputs to `test-files/test-cases.json` and rebuild distributions.
- Update accents, exclusive symbols, or Tamil prefixes in `tool/reader_data.json`.
- Add option metadata to `tool/options.json`, regenerate, and implement its behavior
  in the engine. Regenerate FFI bindings when the export header changes.
- Change byte matcher layout in `packed_trie.h` and scratch/token-key layout in
  `static_trie.h`; preserve bounded matching and source policies. Compare native and Wasm performance and size.

## Related documents

- [Agent guide](AGENTS.md)
- [Performance baseline and reproduction](docs/performance.md)
- [Build and release](docs/release.md)
- [Flutter usage](flutter/README.md)
- [Node.js usage](nodejs/README.md)
- [C exports](native/src/exports.h)
