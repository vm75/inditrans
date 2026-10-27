# Forward mappings and source reading

The canonical arrays in `scripts/script_data.json` describe forward target
representations. Several sounds may share the closest target spelling. Native
source reading is specified separately where those approximations coincide:
`readerOverrides` maps a spelling to a list of checked `type:index` identities;
`readerExclusions` lists writer entries which are approximations rather than
native source spellings. Both are resolved during generation. Readers and writers
remain immutable, with no runtime resolver or phonetic-distance calculation.

The generator rejects unexplained incompatible explicit-source collisions,
partial/unresolved alias targets, out-of-range identities and incomplete writer
inventories. Output-only Indlish has no source-reading guarantee. Duplicate writer
spellings and selected native readings are reported in
`out/static-lookup-collisions.json`. Virtual Mixed Indic keeps Devanagari precedence
for shared spellings, accepts native aliases and canonical equivalents, and excludes Urdu. ASCII
control aliases from individual scripts remain literal in Mixed Indic.

## Inventory review

This review concerns the library's script/romanization profiles. It does not add
language-specific pronunciation models for the languages sharing each script.
Existing Tamil inference and options retain their documented scope. Native
orthographic mappings remain the default; approximations lose source distinctions
when the target inventory lacks them. Tests use explicit forward expectations,
including basic anchors for every canonical writer and selected native readers.

| Canonical profile | Assessment and decisions |
| --- | --- |
| assamese | Native vowels/consonants retained; short e/o approximate native long forms; ae/open e use অ্যা; source এ remains long e. Extended zh/w use ঝ়/ৱ়. |
| bengali | Same vowel approximation policy as Assamese; Bengali r/v source choices remain distinct from Assamese. Extended zh/w use ঝ়/ৱ়. |
| burmese | Native letters retained; ae uses အဲ; remove synthetic short/extended-vowel modifiers; stacks use U+1039, terminal suppression U+103A. |
| devanagari | Native letters/signs retained; candra e reads as ê, independently of Sinhala ae; add affricates, loan letters and visarga allophones. |
| grantha | Unicode Grantha letters/signs, virama and Vedic anusvara signs; uses Tamil digits and shared dandas; Vedic accents are omitted on output. Native e/o read as long ē/ō; unsupported short/open vowels and non-Sanskrit consonants use closest native approximations. Replaces the Malayalam-based Tamil Extended profile. |
| gujarati | Native candra e/o retained; short e/o approximate native long forms; candra e uses the separate ê identity. |
| gurmukhi | Short e/o use native long forms, ae/long ae use ਐ and open o uses ਔ; vocalic r/l use ru/lu approximations; tippi gets its own ISO notation; extended zh uses ਝ਼. Emit adhak for simple geminates after vowels, including k+kh and other unaspirated+aspirated pairs. |
| kannada | Native mappings retained; ae/long ae/open o approximate short e/long e/short o; jihvamuliya/upadhmaniya use native signs. |
| khmer | Native vowels/consonants retained; ae uses អែ; conjuncts use coeng U+17D2, terminal suppression retains U+17D1. |
| malayalam | Native mappings retained; extended vowels use e/ē/o; explicit ഉ്/ു് reads as reduced ŭ; bare candrakkala remains vowel suppression. Add native ഺ. |
| oriya | Extended vowels approximate native e/ē/o; distinguish native ଵ (v) and ୱ (w) when reading and writing; dotted ବ଼ remains a va alias. |
| sinhala | Native short/long ae stay distinct; open o approximates ඔ; no synthetic modifier is needed. |
| tamil | Keep native/Grantha and selected superscript/traditional modes; extended vowels approximate e/ē/o, vocalic r/l use ru/lu; native ru is read as r + u; written word-final stops with pulli are retained. Accept the rare native anusvara sign ஂ without changing ordinary m-based output. |
| telugu | Extended vowels approximate ఎ/ఏ/ఒ and matching signs; real accents remain accents; add native ఁ, ౘ and ౙ with separate identities. |
| thai | Keep the Sanskrit/Pali transliteration inventory; independent vowels and dependent preposed vowels use Unicode visual order; the prefix applies to the whole consonant cluster. |
| tibetan | Palatal c/ch/j/jh use ཅ/ཆ/ཇ/ཇྷ; alveolar ĉ/ĉh/dz use ཙ/ཚ/ཛ. Balti q/ṛ/ṛh use ཫ/ཬ/ཬྷ, retaining older aliases. Use subjoined consonants in stacks, accept fixed subjoined wa/ya/ra and canonical decompositions, and use explicit vowel suppression before extensions without subjoined forms. |
| urdu | New explicit-vowel Perso-Arabic profile; supports aspirates, nasalization, shadda, hamza on yeh and contextual final/medial yeh. See limitations below. |
| hk | Existing scheme inventory and aliases retained; new sounds use existing closest scheme representations. |
| iast | Existing scheme retained, including vocalic r and anusvara notation; new sounds use existing approximations. |
| ipa | New affricate/fricative/approximant and reduced-vowel representations; Urdu loan letters use their Urdu phonetic approximations. |
| iso | All 21 spreadsheet spellings are accepted; new sound/letter identities have directional output coverage as described below. |
| itrans | Existing scheme retained; M, .n and .m read as anusvara; the dot aliases no longer become punctuation plus n. |
| indlish | Output-only readable approximations remain valid; duplicate spellings do not imply recoverable source identities. |
| titus | Existing scheme inventory retained; new sounds use existing closest representations. |
| wx | Existing output role and scheme inventory retained; Vedic accents are omitted on output. |
| slp1 | Preserve K as kh when reading; its existing aytham output approximation does not create a second native reading of K. Vedic accents are omitted on output. |
| velthuis | Existing scheme inventory and aliases retained; new sounds use existing closest representations. |

Grantha mappings follow the [modern Grantha letter tables](https://en.wikipedia.org/wiki/Grantha_script#Modern_Grantha)
and the [Unicode Grantha chart](https://www.unicode.org/charts/PDF/U11300.pdf).
Canonically decomposed o/au vowel signs are accepted. Prenasalized consonant
approximations use native anusvara plus the corresponding stop; they are read as
those native sequences rather than invented single letters.

New identities are appended: vowels 19 (ê), 20 (ŭ) and 21 (long ê), consonants 50–63
(ts, aspirated ts, alveolar t, zh, w, eight Urdu loan-letter distinctions and dz),
and other diacritics 4–7 (two visarga allophones, Telugu short nasal and tippi).
The original indices remain stable. Each writer supplies a representation for
all of them. The 64-consonant lookup capacity is now full; the generator rejects
further growth until that bound is deliberately changed.

## Nontrivial forward choices

The former modifiers `᳴`, fake `॒`, `ʽ`, `ˇ`, dots and separators were not target
vowel/consonant identities. Native approximations now carry their ordinary
reading. Real accents are emitted separately, including consecutive marks; an
additional accent/diacritic is left in the stream rather than overwriting one
already attached to a unit. No replacement separator or reconstruction grammar
is introduced. Literal joiner handling remains unchanged. Literal/protected/XML spans bypass
writer-marker filtering, so special-notation characters inside them remain exact.

For Telugu, Kannada, Malayalam and Tamil, short/long front open vowels use the
available short/long e spellings; open o uses short o. This keeps vowel length
where available, accepts the difference in vowel height, and avoids using a
pitch mark as a vowel modifier. Bengali/Assamese have an established অ্যা/্যা
sequence for ae; short/long ae merge there. Gurmukhi ਐ/ਔ provide closer open
vowels than the former decorated ਏ/ਆ. Burmese အဲ/ဲ and Khmer អែ/ែ supply their
native ae representations. Unsupported vowel length distinctions may merge.

Candra e and Sinhala ae require different ISO output spellings even where a
target approximates both to the same vowel. Therefore `ऍ → ê`, `ඇ → æ`, and
`ඈ → ǣ` have independent source expectations. Thai แอะ/แอ and Devanagari
long candra e use ê/ê̄, preserving their open-e quality and length separately
from Sinhala æ/ǣ. IPA consequently emits ɛ/ɛː for those native open-e inputs. Exact phonetic realization can
vary by language; these identities do not claim a language-independent phonetic
value for every use of a letter.

The comparison spreadsheet's `ŭ` is accepted as a reduced vowel. Unicode
explicitly distinguishes candrakkala's vowel-suppression and neutral-vowel uses.
The explicit Malayalam u-plus-candrakkala sequence selects ŭ; a bare candrakkala
cannot unambiguously identify the pronunciation and keeps its existing virama
reading. Other scripts approximate the reduced vowel with u. IPA uses ɯ.

Voiceless/voiced alveolar affricates are not ordinary palatal c/j. Telugu ౘ/ౙ
and Tibetan ཙ/ཛ have distinct identities; unsupported targets approximate them
with their palatal affricates. Malayalam ഺ likewise gets an alveolar t identity,
with a dental-stop approximation where necessary. Urdu zh and w use the closest
available j/v where a target lacks the distinction. Ain uses IPA ʔ and ISO ʻ;
targets without that sound approximate it with their glottal h. These are
explicit approximations, not additional claims of exact native equivalence.

Gurmukhi emits `akka → ਅੱਕ` and `akkha → ਅੱਖ`: adhak supplies the first
unaspirated half of the following consonant. Initial, longer, terminal and
aspirate+aspirate clusters remain explicit, as do clusters with marks attached
to the first consonant. This keeps the change within the existing reader's
gemination rules. Odia uses its distinct Unicode va/wa letters in both
directions: `ଵ → va`, `ୱ → wa`. See the [Unicode Gurmukhi and Odia descriptions](https://www.unicode.org/versions/Unicode17.0.0/core-spec/chapter-12/).

Tamil pulli removes the inherent vowel, not the consonant. The former blanket
word-final hard-stop deletion lost explicit written sound information, including
loan and sandhi spellings. Written stops are retained; contextual voicing and
its existing opt-out remain in place. See the [Unicode Tamil description](https://www.unicode.org/versions/Unicode17.0.0/core-spec/chapter-12/).

## Spreadsheet notation coverage

The spreadsheet is an inventory of requested spellings, not a conformance test.
Its Kaithi/Saurashtra columns do not imply those scripts are supported. The
published ISO 15919 scope also differs from the table's entire comparison.

| Spellings | Implemented forward behavior |
| --- | --- |
| ê, ŭ | Separate vowel identities, with corresponding dependent signs; long open-e uses the additional ê̄ extension. |
| ṃ | ISO tippi identity; native Gurmukhi ੰ emits ṃ. Ordinary ISO anusvara stays ṁ. |
| n̆ | Separate Telugu short nasal sign ఁ. |
| ẖ, ḫ | Separate visarga allophones; ᳵ/ᳶ and Kannada equivalents no longer read as avagraha. |
| ĉ, ĉh, ṯ, ž, w | Separate consonant identities, native spellings where supported, approximations elsewhere. |
| s̱, s̤, h̤, t̤, ʻ, ẕ, ż, ẓ | Distinct Urdu loan-letter notation; other targets may merge their phonetic representations. |
| r̆ | Accepted as r + explicit vowel suppression. Output follows the target's ordinary dead-r/conjunct rules; a special written ra shape is not preserved. |
| ' | Accepted avagraha alias; ISO output keeps its existing curly apostrophe ’. |

Complete ISO loan-letter spellings take precedence over interpreting their
combining marks as pitch accents: `s̱a` is a loan-letter syllable, while `sa̱`
retains the accent on its vowel. Tippi keeps anusvara assimilation behavior
under the existing inference options.

The additional ISO-profile outputs `ê̄` and `dz` are extensions: `ê̄` represents
long open-e, while `dz` is an ordinary consonant sequence for
readers; IPA d͡z and native Telugu/Tibetan affricates carry the precise new
identity. This extension is not a claim that all additional representations
are normative ISO 15919 spellings.

Thai sara a is visarga in the Sanskrit/Pali profile. After preposed e/o/ae it
shortens the corresponding vowel; without a preposed vowel, aa plus sara a
reads as long aa plus visarga. Some Thai vowel/visarga sequences are inherently
ambiguous across orthographic conventions; the profile does not infer a language
or pronunciation from a dictionary.

## Urdu and Nastaliq

`urdu` is the canonical profile and `nastaliq` its discoverability alias. The
profile implements Urdu/Perso-Arabic Unicode text, not every language written in
Nastaliq. Rendering uses a Nastaliq font; bytes stay in logical order. No Arabic
presentation forms or inserted direction controls are emitted.

Independent vowels use carriers, dependent vowels use marks/matres, and inherent
canonical a emits fatha. Explicit vowel suppression emits sukun. Shadda expands
to a geminate when reading and can be emitted for repeated consonants. Aspirates
accept a short-vowel mark before or after do-chashmi-he. Medial e uses yeh and
word-final e uses bari yeh; i/ī and e hiatus use hamza on yeh. These are narrow script
rules, not a dictionary-based Urdu pronunciation or spelling system.

Bare consonants have no guessed vowel. Bare alif defaults to long a. Ambiguous bare waw defaults to consonantal
v; yeh defaults to y, while bari yeh defaults to long e. Ordinary unvocalized Urdu
needs lexical knowledge to recover omitted vowels and many uses of these letters.
Use explicit vowels for reliable source conversion. Not every context-sensitive
hamza spelling or Arabic grammatical mark is interpreted. Unrecognized text
continues through the existing literal-text policy.

## Evidence and references

- [Unicode South Asian scripts](https://www.unicode.org/versions/Unicode17.0.0/core-spec/chapter-12/): native characters, combining sequences, candrakkala and script shaping.
- [Unicode additional scripts](https://www.unicode.org/versions/Unicode17.0.0/core-spec/chapter-13/): Tibetan stacks and Sinhala vowels.
- [Unicode Southeast Asian scripts](https://www.unicode.org/versions/Unicode17.0.0/core-spec/chapter-16/): Thai order, Myanmar stacking and Khmer coeng.
- [Unicode Arabic](https://www.unicode.org/versions/Unicode17.0.0/core-spec/chapter-9/) and [Arabic FAQ](https://www.unicode.org/faq/arabic.html): logical storage, joining, marks and presentation forms.
- [Library of Congress Urdu table](https://www.loc.gov/catdir/cpso/romanization/urdu.pdf): letter/vowel/aspirate conventions and the need for dictionary information when vowels are missing. It is not an ISO spelling oracle.
- [ISO published scope](https://www.iso.org/standard/28333.html): no conformance claim is inferred from the spreadsheet alone.

Shared directional regressions live in `test-files/test-cases.json`; generator
checks live in `scripts/tests/test_phonetic_data.py`. Font source hashes and the
shaping corpus are in `scripts/verify_demo_font.py`. Rendering validation reports
must distinguish the tested font/browser combination from arbitrary system fonts
and platforms.
