# Urdu lexicon preparation

## Generic wordlist-driven pronunciation pipeline

**The lexicon builder is source-neutral and no longer has a 3 MB size cap.**
IUL1 retains a 32-bit entry count and 16-bit *per-word/per-reading* lengths.
Runtime memory and the target platform's address space are the practical
limits. The CLI accepts arbitrary valid IUL1 files with `--urdu-lexicon`,
and the native/WASM default pack is regenerated from a wordlist at build time.

Wordlists use UTF-8 tab-separated rows:

```text
# word<TAB>pronunciation<TAB>notation<TAB>priority
پل	pal	iast	100
پل	pul	iast	20
کتاب	kitāb	iast	100
```

Accepted notation is `iast` (canonical Inditrans Roman) or `ipa` (supported
space-separated phonemes). A pronunciation is **required for an active entry**;
a bare Urdu word without pronunciation can be retained as an unresolved
candidate for later review but does not cause invented vowels. Rows with
unrepresentable phonemes are rejected or, with `--strict`, fail the build.
Duplicate spellings with different readings are *kept*. Highest explicit
editorial priority determines the default, with deterministic token-order
tie-breaking; all other supported readings remain in IUL1. Priority is NOT a
statistically measured frequency unless you supply and document actual counts.
No neighboring-word lookup, grammar analysis or trained language model runs
at runtime.

The current **expanded experimental candidate wordlist**, stored at
`scripts/urdu/wordlists/wikipron-candidates.tsv`, derives from the complete
7,709-row WikiPron Urdu snapshot (6,296 distinct raw spellings before phoneme
validation), **not** a new independently authored dictionary.
`scripts/urdu/wordlists/editorial-defaults.tsv` contains selected authored
readings (including `پل → pal`, with `pul` retained) and the demo examples.
The exact number of supported pronunciations, omissions, ambiguous spellings
and packed bytes is recorded in `scripts/urdu/wordlist-report.json`.

To rebuild the current branch's expanded pack:

```sh
python3 scripts/urdu/build_wordlist.py \\
  scripts/urdu/wordlists/wikipron-candidates.tsv \\
  scripts/urdu/wordlists/editorial-defaults.tsv \\
  --version urdu-wordlist-experimental-2 \\
  --provenance 'WikiPron Urdu 2026 candidate + Inditrans editorial defaults' \\
  --license 'CC BY-SA 4.0 (Wiktionary-derived candidate data)' \\
  --out demo/urdu-pronunciation.iul \\
  --report scripts/urdu/wordlist-report.json
python3 scripts/urdu/embed_lexicon.py
python3 scripts/urdu/embed_lexicon.py --check
make wasm
```

For your later curated shortlist, pass `--selection PATH` (one Urdu word per
line), or `--limit N` to choose a fixed number of entries using the declared
editorial priority. You can **replace both wordlist TSVs entirely** with your
own independently collected dictionary and provide its actual license,
version, and provenance. The build and runtime do not require WikiPron.

**Licensing is about the data, not the compiler:** the current branch's
embedded corpus *still* derives from Wiktionary and is distributed as CC BY-SA
4.0, with attribution to Wiktionary contributors and the WikiPron project
(Lee et al., *Massively Multilingual Pronunciation Mining with WikiPron*,
LREC 2020). It is not automatically relicensed MIT by passing it through
Inditrans or changing the build format. An independently licensed distribution
requires separately sourced and reviewed data, removal/replacement of the
current derived asset and regenerated native/WASM binaries. The original
Platts (1884) dictionary is a promising public-domain *print original*, but
rights to any modern digital transcription must be checked before importing.
The current corpus has **not** been independently pronunciation-reviewed.

### Legacy concordance experiment

The earlier 4,252-entry `build_concordant.py` experiment and its historical
`scripts/urdu/concordant-report.json` are kept for comparison. They are
not the default embedded asset. The source SHA pinning and strict concordance
rules remain useful as a more conservative comparison, but there is no longer
a fixed total-byte limit. The old 452-byte `demo/urdu-example.iul` is retained
as an isolated test fixture. To force orthographic output, pass
`-o DisableInference` or check **Disable inference** in the demo.


`build_lexicon.py SOURCE OUTPUT` validates and sorts a version 1 JSON source
into a deterministic UTF-8 JSON artifact. The artifact records the SHA-256 of
the exact source. With `--binary`, it instead emits a portable `IUL1` pack,
including version, provenance and source hash metadata. The experimental C++
adapter accepts this pack as caller-owned bytes; it does not read files. It
falls back on explicitly marked input words, even when an entry has those marks.
The pack uses a CRC-32 to detect accidental corruption; this is not an
authenticity or trust check. A failed decode uses the ordinary Urdu reader.

The source must contain `format: 1`, a `version`, a `provenance` description
(including the source's license and attribution), and `entries`. Each entry has
an Urdu `word` and an ordered array of `readings`. The first reading is the
default; subsequent readings are alternatives, not contextual predictions.
Each reading is an array of units. A unit has either `v` (independent vowel) or
`c` (consonant), optionally `m` (vowel mark, consonants only) and `o` (other
diacritic). Values are canonical indices in `scripts/script_data.json`. An
absent `m` on a consonant denotes inherent a; `m: 0` is virama. The indices
are not Unicode code points or target-specific strings.

Keys are NFC-normalized and the existing Urdu reader's Arabic kaf, yeh and heh
equivalents are mapped to their Urdu forms. Marks are retained. Collisions and
duplicate readings are errors, so data disagreements require explicit review.
The binary reader also rejects duplicate, unsorted or alias-bearing keys rather
than selecting an arbitrary entry from a malformed pack.
Do not add a dataset until its individual files' redistribution terms and
pronunciation conventions have been reviewed. No production lexicon is bundled.

The [Dakshina dataset](https://github.com/google-research-datasets/dakshina)
offers Urdu word/romanization pairs annotated by speakers and states that the
dataset is CC BY-SA 4.0. Its Roman spellings are not canonical phoneme labels;
conversion and review are still required before they can be used here. Its
Wikipedia-derived data and share-alike terms also need a distribution review.

[Zuhri](https://huggingface.co/datasets/humair025/zuhri) describes a verified
Urdu phoneme dataset and labels it Apache-2.0, but its page refers to a LICENSE
file that is absent from its listed files. A separate, newer dataset by the same
author is labelled non-commercial. The exact data's rights and the phoneme-to-
token conversion need review before it can be included in a production pack.
The inspected `advnc_output-28k.csv` (SHA-256
`9e50cf6170dff3bc1bb4adf4882af8ef2bfe437768de9fc6d0780cc21a7068f2`)
contains 16,323 rows, including duplicate spellings and non-phoneme Urdu
characters in the phoneme field. The `is_valid` column marks almost every row
true, so it alone is insufficient evidence for automatic inclusion. The source
was inspected locally and is not redistributed here.

`python3 scripts/urdu/import_zuhri.py INPUT.csv OUTPUT.json` creates an
**evaluation candidate**, not a reviewed or distributable production lexicon.
It maps only the explicitly listed IPA approximations to existing token indices;
unknown phonemes, marked/unsupported words, and conflicting normalized
spellings are omitted. Unsupported variants cause the entire spelling to be
omitted. For the CSV identified above, this produced 8,054 candidate spellings
and excluded 37 conflicting spellings; these counts do not measure accuracy or
prove the source's redistribution rights. Review the mapping, upstream data and
licensing before distributing a derived pack. Feed the JSON into
`build_lexicon.py` to exercise the resolver offline.
For example, that candidate pack produces `urdū`, `hukūmat`, and `salām` for
`اردو`, `حکومت`, and `سلام` through the IAST writer. These examples are not a
held-out accuracy evaluation.

## Evaluation

`python3 scripts/urdu/evaluate_lexicon.py CANDIDATE.json REFERENCE.json` reports
word coverage, ambiguity, selected-reading exact matches and phoneme edit counts
for covered isolated words. Reference labels must be independently prepared and
must not be used to generate or tune the candidate. The tool does not measure
sentence accuracy, fallback output, false corrections in running text, or the
accuracy of its own IPA-to-token mapping.

`reference-pilot.json` is a five-word *draft*, not a human-reviewed gold set.
Its token conversions were prepared for this evaluation and need independent
linguistic review. The underlying pronunciations are attributed to Wiktionary
contributors, [CC BY-SA 4.0](https://creativecommons.org/licenses/by-sa/4.0/):
[اردو](https://en.wiktionary.org/w/index.php?title=اردو&oldid=93423094),
[کتاب](https://en.wiktionary.org/w/index.php?title=کتاب&oldid=93581375),
[حکومت](https://en.wiktionary.org/w/index.php?title=حکومت&oldid=89423381),
[سلام](https://en.wiktionary.org/w/index.php?title=سلام&oldid=93627038), and
[کل](https://en.wiktionary.org/w/index.php?title=کل&oldid=92378809).
The reference file is provided under CC BY-SA 4.0. Its labels were not used to
build the Zuhri candidate, but three words were selected after observing
candidate hits, so the sample is biased. Against the pinned candidate, the
pilot reports three covered words, all exact, and two OOV, including ambiguous
`کل`. Five words are far too few to establish quality; a larger reviewed word and sentence set is still
required before distributing a production lexicon.

`python3 scripts/urdu/evaluate_runtime.py native/build_linux/libinditrans.so CANDIDATE.json REFERENCE.json`
compares actual literal and assisted engine output against each reference reading
rendered by the same target writer. Use `--target` to select a target script.
It reports covered words, exact outputs, corrections, false corrections, and
unsupported reference words. The reference is encoded separately in memory and
is not added to the candidate. On the biased five-word pilot, literal IAST
matched none of the five words and the Zuhri candidate matched three, with zero
false corrections; this is a functional example, not a general quality claim.

`python3 scripts/urdu/evaluate_sentences.py native/build_linux/libinditrans.so CANDIDATE.json REFERENCE.json`
compares whole output strings in context. Each sentence reference supplies one
chosen token reading per eligible word outside XML tags and `##…##` spans;
the tool rejects missing, extra, or ambiguous annotations. Sentences containing
unsupported or mixed Urdu spans, and sentences with no eligible Urdu words,
are excluded from the scored totals rather than treated as verified fallback.
Other text uses the legacy reader even in the reference output, so this is not
a complete phonetic assessment of mixed or explicitly marked text.
`sentence-pilot.json` has three synthetic, unreviewed contexts derived from
the same draft word references and is likewise CC BY-SA 4.0. For the pinned
Zuhri candidate it produces two exact outputs versus none for the literal reader;
the third remains incorrect because `کل` is OOV. This tests the evaluator and
preservation behavior, not sentence-level quality on a representative corpus.

### Human Roman-spelling agreement screen

`python3 scripts/urdu/evaluate_dakshina.py native/build_linux/libinditrans.so CANDIDATE.json REFERENCE.tsv`
compares actual IAST output with the human Roman spellings in a local Dakshina
Urdu lexicon TSV. It folds case and Latin diacritics and collapses repeated
identical vowels; it does not infer a standardized phoneme transcription from
the annotators' varied spelling conventions. A word agrees if the folded output
matches any of its folded annotations. This is a spelling-agreement screen,
not phoneme accuracy, sentence accuracy, or a false-correction rate. Multiple
spellings often remain after folding, and lost agreement does not prove that a
pronunciation was incorrect.

The inspected [Dakshina v1.0 release](https://github.com/google-research-datasets/dakshina)
is CC BY-SA 4.0 and contains human-annotated Romanizations. We extracted only the
Urdu development and test TSVs from the published archive generation
`1590618303711280`; they are not redistributed here. The development TSV's
SHA-256 is `5ad67a8f4a2bdecd69c50cdd522880c6b4f5da2e35f5f29930b699bae7eb1f71`;
the test TSV's is `c57a804a5f7bf0e51486c0a8258ec41ed62ebc4612aed741e0f3bff47d703fd8`.
The candidate was generated from WikiPron and Wiktextract without these
annotations. Both ecosystems involve Wikimedia material, so full independence
of the underlying content cannot be assumed.

With the 4,248-word concordant candidate described below (JSON SHA-256
`b99d1faf972bf588dde5847d82affb8efccd4fb35058dde245be562c30350717`), the development
set had 2,178 eligible normalized words, 223 covered, with 19 covered literal
outputs and 146 covered assisted outputs agreeing with at least one folded
annotation. The test set had 2,157 eligible words, 220 covered, with 32 covered
literal outputs and 141 covered assisted outputs agreeing. Across all eligible
words, the development set gained agreement for 131 and lost it for 4; the test
set gained it for 111 and lost it for 2. These figures show a useful change in
this narrow screen, but do not replace independent human-reviewed canonical
phoneme and sentence references or justify a production release on their own.

`python3 scripts/urdu/evaluate_dakshina_sentences.py LIBRARY CANDIDATE.json REJOINED.tsv`
applies a similarly limited folded-spelling comparison to the human-romanized
sentence TSV. It preserves word boundaries, folds decimal digits, and excludes
sentences containing unsupported Urdu spans or non-Latin output. On the pinned
`ur.romanized.rejoined.tsv` (SHA-256
`018202e0ae75393bafa23fe333ef8060946081d0bb1a56180126df72c3120148`),
8,048 of 9,759 sentences contained unsupported Urdu spans and 23 more produced
unscorable non-Latin output. Of the remaining 1,688, 1,019 had at least one
candidate word. Literal output agreed with the folded annotation for 10
sentences and assisted output for 50; 41 gained agreement and one lost it.
There were no byte-level output changes in scored or non-Latin-output cases
without a lexicon hit. This biased subset and its Roman spelling comparison do
not establish sentence-level phoneme accuracy; the excluded cases remain an
important limitation.

## Wiktextract comparison

`python3 scripts/urdu/import_wiktextract.py INPUT.jsonl OUTPUT.json` prepares a
second evaluation candidate from Urdu Wiktextract JSONL. It maps only supported
IPA symbols, omits unsupported regional or optional pronunciations, and rejects
spellings with conflicting readings or a sense without a pronunciation. It does
not strip written vowel marks. Review the extract's per-file rights and its
phoneme mappings before distributing a derivative.

The inspected [Urdu extract](https://kaikki.org/dictionary/Urdu/index.html)
identifies an English Wiktionary dump dated 2026-10-01 and a Wiktextract build
dated 2026-10-25. The downloaded postprocessed JSONL had SHA-256
`6fef1d227ce1cbc309d38f7475e502bc0f6335dc63f4133c7757c44ed2271ec9`.
Its page says postprocessing can merge other sources, so the rights of the
specific pronunciation fields still need verification. The inspected extract
is not redistributed here. The importer produced 4,505 candidate spellings.

Comparing the Zuhri and Wiktextract candidates with `evaluate_lexicon.py`
found 648 shared eligible words; 193 of those had identical mapped phoneme
sequences. Differences include initial short vowels and aspiration. This is
cross-source agreement, not accuracy: the sources may share material, the IPA
conversions are approximations, and neither candidate is a reviewed gold set.
The pilot reference above also comes from Wiktionary, so it cannot independently
evaluate the Wiktextract candidate. These results do not justify shipping either
candidate as a production lexicon.

## WikiPron candidate

`python3 scripts/urdu/import_wikipron.py INPUT.tsv OUTPUT.json` converts the
Urdu broad-IPA TSV from [WikiPron](https://github.com/CUNY-CL/wikipron). WikiPron
licenses its code under Apache-2.0 and explicitly says its Wiktionary-derived
data has [separate terms](https://en.wiktionary.org/wiki/Wiktionary:Copyrights);
do not assume the code license applies to the data. The inspected source was
`data/scrape/tsv/urd_arab_broad.tsv` at commit
`1b6fce9d894e38eb65431676e4b2fcfc3db19db8`, SHA-256
`5530b71670423fc810e245680a9077ca3c72c79de0cdf6d2df21bbec5130d1e5`.
It is not redistributed here. Cite Lee et al., *Massively Multilingual
Pronunciation Mining with WikiPron* (LREC 2020), and preserve Wiktionary
attribution and applicable share-alike terms when distributing derivatives.

The importer retains explicitly mapped phonemes, including mapped aspiration
and nasalization; unsupported or conflicting variants cause abstention. With
the inspected TSV it emits 4,673 candidate spellings. WikiPron's TSV does not
include untranscribed senses, so this alone can miss homographs such as `کل`.
The optional `--confirm-with WIKTEXTRACT_CANDIDATE.json` retains only identical
single readings also present in the conservative Wiktextract candidate above;
with the inspected files this produced 4,248 candidates and rejected 425.
Both inputs derive from Wiktionary: their agreement checks extraction and
mapping consistency, not independent pronunciation accuracy or complete
homograph coverage. Both hashes are recorded in the candidate provenance. A
reviewed reference set and redistribution review are still needed before a
production pack is shipped.

## Reusable lexicon lifecycle (development branch)

The native C ABI now supports `createUrduLexiconHandle(data, length)`,
`transliterateUrduWithLexiconHandle(text, to, options, handle, skipStart, skipEnd)`,
and `releaseUrduLexiconHandle(handle)`. A loaded handle owns its compact key
and phoneme tables; callers may discard the original IUL1 buffer after loading.
The old `transliterateUrduWithLexiconData` remains for one-shot calls and
preserves legacy fallback behavior for invalid assets. The new API avoids
rechecking CRC, decoding tokens, or validating the entire vocabulary on every
conversion. Handles are immutable after loading and must not be released while
a conversion is running. The caller owns and releases every result buffer.

Words are looked up via binary search of strictly sorted NFC/alias-normalized
keys. The current pack preserves only the default pronunciation; alternative
readings are parsed/validated but not surfaced. No language-model-based
contextual disambiguation is included. The native C++ span API likewise requires
strictly sorted canonical lexicon keys.

Build the native library and WASM artifacts after changing exports; the checked-in
binaries must be regenerated before platform integration tests can pass.
The 3 MB optional data budget and 4 MiB incremental runtime-memory target
still require representative full-size benchmarks and a legally distributable
production lexicon. The current pack is only a regression fixture.

## Example-only proverb pack

`demo/urdu-example.iul` is a **452-byte, project-authored demonstration
asset** generated reproducibly from `scripts/urdu/demo-lexicon.json`:

```sh
python3 scripts/urdu/build_lexicon.py scripts/urdu/demo-lexicon.json demo/urdu-example.iul --binary
```

It recognizes exactly five spellings: `جیسی`, `کرنی`, `ویسی`,
`بھرنی`, and `کتاب`. It is provided to test integration of the new
opt-in reader in the demo and native CLI, not as a general Urdu lexicon.
For example, enable **Urdu pronunciation (sample words)** in the hosted
demo conversion options, or use:

```sh
make out/inditrans
out/inditrans -f urdu -t indlish -l demo/urdu-example.iul 'جیسی کرنی ویسی بھرنی۔'
```

For the words in this pack, the output is `jaisI karnI vaisI bharnI`
followed by Inditrans's existing full-stop mapping. The core
`transliterate()` API and CLI invocation **without** `-l` retain their
legacy orthographic behavior. Caller-provided larger packs can reuse the same
CLI option and C API; no external runtime dependency is needed.

The demo pack is illustrative and has no corpus-wide accuracy, coverage, or
licensing claims for imported datasets. It does **not** satisfy issue #70's
requirement for a reviewed, redistributable production pronunciation lexicon.
