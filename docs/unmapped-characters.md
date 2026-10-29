# Unmapped source characters: audit and possible mappings

This is an inventory of **standalone code points with no direct entry in the generated source reader**, not a claim that every listed character should be treated as an ordinary letter. Existing multi-character entries may recognize a sequence containing a listed code point, and a listed character may belong to another language using the same Unicode block. Unmapped text currently passes through; same-script conversion may copy the input unchanged.

The inventory covers the 17 native source readers and the main Unicode blocks listed below, using Python Unicode data version **15.1.0** and the reader generated from `scripts/script_data.json` and `scripts/reader_data.json` in this checkout. Assamese and Bengali intentionally share a block. Urdu includes the Arabic, Arabic Supplement and Arabic Extended-A blocks; this is a broad candidate set, not a claim that every character is used in Urdu. Roman sources, Unicode blocks outside the ranges below, unassigned characters, controls, separators, combining sequences and characters added after this Unicode version are outside this inventory. Consequently this is not a complete inventory of every character a user might enter.

**★** marks an entry included in the CLDR main-letter or punctuation exemplar set for the language of that reader, as a practical indicator of common modern use. Khmer `ៗ` is also marked based on its documented use as a repetition sign. The star is not a frequency measurement: some CLDR main sets include less frequent characters, punctuation usage varies, and unstarred entries have not been established as rare. This review covers Bengali, Burmese, Hindi (Devanagari), Odia, Sinhala, Tamil, Thai, Tibetan, Khmer and Urdu only; other readers and languages sharing their blocks still need review. Exemplar characters can also be recognized as part of a longer sequence even when listed here without a standalone reader entry.

Sources: [CLDR exemplar definitions](https://cldr.unicode.org/translation/core-data/exemplars), CLDR 47 data for [Bengali](https://raw.githubusercontent.com/unicode-org/cldr/release-47/common/main/bn.xml), [Hindi](https://raw.githubusercontent.com/unicode-org/cldr/release-47/common/main/hi.xml), [Odia](https://raw.githubusercontent.com/unicode-org/cldr/release-47/common/main/or.xml), [Sinhala](https://raw.githubusercontent.com/unicode-org/cldr/release-47/common/main/si.xml), [Tamil](https://raw.githubusercontent.com/unicode-org/cldr/release-47/common/main/ta.xml), [Thai](https://raw.githubusercontent.com/unicode-org/cldr/release-47/common/main/th.xml), [Khmer](https://raw.githubusercontent.com/unicode-org/cldr/release-47/common/main/km.xml), and [Urdu](https://raw.githubusercontent.com/unicode-org/cldr/release-47/common/main/ur.xml); [Khmer repetition sign](https://www.w3.org/TR/khmr-lreq/). [Burmese](https://unicode.org/cldr/charts/41/summary/my.html) and [Tibetan](https://unicode.org/cldr/charts/41/summary/bo.html) use CLDR 41. The Vedic comparison for `ৼ` and `ꣳ` follows the [Unicode encoding proposal](https://www.unicode.org/L2/L2015/15161r-bengali-vedic-anusvara.pdf).

**Closest** means a proposed fallback, not an established identity. “Not determined” is an explicit research item: Unicode names alone do not establish a reliable pronunciation or target equivalent. Numeric values are character values, not necessarily a valid conversion of a whole number written with that character. **Solution** is a possible implementation, not an existing behavior. Verify each proposal for its language, all target scripts, neighboring characters and mixed `indic` input before changing the canonical data. Never replace word repetition with a fixed character alias.

Useful sources: [Unicode code charts](https://www.unicode.org/charts/), [Khmer names list](https://unicode.org/Public/18.0.0/charts/nameslist/1780/), [Tibetan names list](https://www.unicode.org/Public/18.0.0/charts/nameslist/0f00/), and the existing [mapping review](phonetic-mappings.md). For project policy and current coverage, see the [README](../README.md#symbol-coverage-and-limitations).

**Implementation categories:** a data alias uses `equivalents` or `readerOverrides` in `scripts/script_data.json` and regenerated immutable tables; a new token requires target mappings; context handling needs an engine change. All changes need shared cases, documentation of lossy mappings and rebuilt WASM assets. For repeated and combined numerals, the value of a character alone does not determine the value of the full sequence.

## Assamese (15 candidates)

Blocks: U+0980–U+09FF.

| Code point | Character | Unicode name | Closest candidate | Possible Inditrans solution |
|---|---|---|---|---|
| U+0980 | `ঀ` | BENGALI ANJI | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+09BC | `◌়` | BENGALI SIGN NUKTA | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+09D7 | `◌ৗ` | BENGALI AU LENGTH MARK | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+09F2 | `৲` | BENGALI RUPEE MARK | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+09F3 | `৳` | BENGALI RUPEE SIGN | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+09F4 | `৴` | BENGALI CURRENCY NUMERATOR ONE | Numeric value `1/16` | Add numeric conversion; check neighboring numerals and target representation. |
| U+09F5 | `৵` | BENGALI CURRENCY NUMERATOR TWO | Numeric value `1/8` | Add numeric conversion; check neighboring numerals and target representation. |
| U+09F6 | `৶` | BENGALI CURRENCY NUMERATOR THREE | Numeric value `3/16` | Add numeric conversion; check neighboring numerals and target representation. |
| U+09F7 | `৷` | BENGALI CURRENCY NUMERATOR FOUR | Numeric value `1/4` | Add numeric conversion; check neighboring numerals and target representation. |
| U+09F8 | `৸` | BENGALI CURRENCY NUMERATOR ONE LESS THAN THE DENOMINATOR | Numeric value `3/4` | Add numeric conversion; check neighboring numerals and target representation. |
| U+09F9 | `৹` | BENGALI CURRENCY DENOMINATOR SIXTEEN | Numeric value `16` | Add numeric conversion; check neighboring numerals and target representation. |
| U+09FA | `৺` | BENGALI ISSHAR | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+09FB | `৻` | BENGALI GANDA MARK | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+09FC | `ৼ` | BENGALI LETTER VEDIC ANUSVARA | `ꣳ` (Vedic anusvara) | Map to the existing Vedic sign token; verify tradition-specific distinctions. |
| U+09FE | `◌৾` | BENGALI SANDHI MARK | Not determined | Review mark function; add a diacritic alias or contextual handling. |

## Bengali (15 candidates)

Blocks: U+0980–U+09FF.

| Code point | Character | Unicode name | Closest candidate | Possible Inditrans solution |
|---|---|---|---|---|
| U+0980 | `ঀ` | BENGALI ANJI | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+09BC | `◌়` | BENGALI SIGN NUKTA **★** | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+09D7 | `◌ৗ` | BENGALI AU LENGTH MARK **★** | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+09F2 | `৲` | BENGALI RUPEE MARK | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+09F3 | `৳` | BENGALI RUPEE SIGN | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+09F4 | `৴` | BENGALI CURRENCY NUMERATOR ONE | Numeric value `1/16` | Add numeric conversion; check neighboring numerals and target representation. |
| U+09F5 | `৵` | BENGALI CURRENCY NUMERATOR TWO | Numeric value `1/8` | Add numeric conversion; check neighboring numerals and target representation. |
| U+09F6 | `৶` | BENGALI CURRENCY NUMERATOR THREE | Numeric value `3/16` | Add numeric conversion; check neighboring numerals and target representation. |
| U+09F7 | `৷` | BENGALI CURRENCY NUMERATOR FOUR | Numeric value `1/4` | Add numeric conversion; check neighboring numerals and target representation. |
| U+09F8 | `৸` | BENGALI CURRENCY NUMERATOR ONE LESS THAN THE DENOMINATOR | Numeric value `3/4` | Add numeric conversion; check neighboring numerals and target representation. |
| U+09F9 | `৹` | BENGALI CURRENCY DENOMINATOR SIXTEEN | Numeric value `16` | Add numeric conversion; check neighboring numerals and target representation. |
| U+09FA | `৺` | BENGALI ISSHAR **★** | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+09FB | `৻` | BENGALI GANDA MARK | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+09FC | `ৼ` | BENGALI LETTER VEDIC ANUSVARA | `ꣳ` (Vedic anusvara) | Map to the existing Vedic sign token; verify tradition-specific distinctions. |
| U+09FE | `◌৾` | BENGALI SANDHI MARK | Not determined | Review mark function; add a diacritic alias or contextual handling. |

## Burmese (123 candidates)

Blocks: U+1000–U+109F, U+AA60–U+AA7F, U+A9E0–U+A9FF.

| Code point | Character | Unicode name | Closest candidate | Possible Inditrans solution |
|---|---|---|---|---|
| U+105A | `ၚ` | MYANMAR LETTER MON NGA | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+105B | `ၛ` | MYANMAR LETTER MON JHA | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+105C | `ၜ` | MYANMAR LETTER MON BBA | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+105D | `ၝ` | MYANMAR LETTER MON BBE | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+105E | `◌ၞ` | MYANMAR CONSONANT SIGN MON MEDIAL NA | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+105F | `◌ၟ` | MYANMAR CONSONANT SIGN MON MEDIAL MA | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+1060 | `◌ၠ` | MYANMAR CONSONANT SIGN MON MEDIAL LA | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+1061 | `ၡ` | MYANMAR LETTER SGAW KAREN SHA | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+1062 | `◌ၢ` | MYANMAR VOWEL SIGN SGAW KAREN EU | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+1063 | `◌ၣ` | MYANMAR TONE MARK SGAW KAREN HATHI | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+1064 | `◌ၤ` | MYANMAR TONE MARK SGAW KAREN KE PHO | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+1065 | `ၥ` | MYANMAR LETTER WESTERN PWO KAREN THA | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+1066 | `ၦ` | MYANMAR LETTER WESTERN PWO KAREN PWA | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+1067 | `◌ၧ` | MYANMAR VOWEL SIGN WESTERN PWO KAREN EU | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+1068 | `◌ၨ` | MYANMAR VOWEL SIGN WESTERN PWO KAREN UE | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+1069 | `◌ၩ` | MYANMAR SIGN WESTERN PWO KAREN TONE-1 | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+106A | `◌ၪ` | MYANMAR SIGN WESTERN PWO KAREN TONE-2 | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+106B | `◌ၫ` | MYANMAR SIGN WESTERN PWO KAREN TONE-3 | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+106C | `◌ၬ` | MYANMAR SIGN WESTERN PWO KAREN TONE-4 | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+106D | `◌ၭ` | MYANMAR SIGN WESTERN PWO KAREN TONE-5 | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+106E | `ၮ` | MYANMAR LETTER EASTERN PWO KAREN NNA | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+106F | `ၯ` | MYANMAR LETTER EASTERN PWO KAREN YWA | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+1070 | `ၰ` | MYANMAR LETTER EASTERN PWO KAREN GHWA | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+1071 | `◌ၱ` | MYANMAR VOWEL SIGN GEBA KAREN I | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+1072 | `◌ၲ` | MYANMAR VOWEL SIGN KAYAH OE | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+1073 | `◌ၳ` | MYANMAR VOWEL SIGN KAYAH U | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+1074 | `◌ၴ` | MYANMAR VOWEL SIGN KAYAH EE | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+1075 | `ၵ` | MYANMAR LETTER SHAN KA | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+1076 | `ၶ` | MYANMAR LETTER SHAN KHA | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+1077 | `ၷ` | MYANMAR LETTER SHAN GA | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+1078 | `ၸ` | MYANMAR LETTER SHAN CA | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+1079 | `ၹ` | MYANMAR LETTER SHAN ZA | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+107A | `ၺ` | MYANMAR LETTER SHAN NYA | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+107B | `ၻ` | MYANMAR LETTER SHAN DA | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+107C | `ၼ` | MYANMAR LETTER SHAN NA | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+107D | `ၽ` | MYANMAR LETTER SHAN PHA | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+107E | `ၾ` | MYANMAR LETTER SHAN FA | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+107F | `ၿ` | MYANMAR LETTER SHAN BA | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+1080 | `ႀ` | MYANMAR LETTER SHAN THA | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+1081 | `ႁ` | MYANMAR LETTER SHAN HA | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+1082 | `◌ႂ` | MYANMAR CONSONANT SIGN SHAN MEDIAL WA | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+1083 | `◌ႃ` | MYANMAR VOWEL SIGN SHAN AA | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+1084 | `◌ႄ` | MYANMAR VOWEL SIGN SHAN E | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+1085 | `◌ႅ` | MYANMAR VOWEL SIGN SHAN E ABOVE | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+1086 | `◌ႆ` | MYANMAR VOWEL SIGN SHAN FINAL Y | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+1087 | `◌ႇ` | MYANMAR SIGN SHAN TONE-2 | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+1088 | `◌ႈ` | MYANMAR SIGN SHAN TONE-3 | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+1089 | `◌ႉ` | MYANMAR SIGN SHAN TONE-5 | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+108A | `◌ႊ` | MYANMAR SIGN SHAN TONE-6 | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+108B | `◌ႋ` | MYANMAR SIGN SHAN COUNCIL TONE-2 | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+108C | `◌ႌ` | MYANMAR SIGN SHAN COUNCIL TONE-3 | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+108D | `◌ႍ` | MYANMAR SIGN SHAN COUNCIL EMPHATIC TONE | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+108E | `ႎ` | MYANMAR LETTER RUMAI PALAUNG FA | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+108F | `◌ႏ` | MYANMAR SIGN RUMAI PALAUNG TONE-5 | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+109A | `◌ႚ` | MYANMAR SIGN KHAMTI TONE-1 | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+109B | `◌ႛ` | MYANMAR SIGN KHAMTI TONE-3 | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+109C | `◌ႜ` | MYANMAR VOWEL SIGN AITON A | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+109D | `◌ႝ` | MYANMAR VOWEL SIGN AITON AI | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+109E | `႞` | MYANMAR SYMBOL SHAN ONE | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+109F | `႟` | MYANMAR SYMBOL SHAN EXCLAMATION | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+AA60 | `ꩠ` | MYANMAR LETTER KHAMTI GA | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+AA61 | `ꩡ` | MYANMAR LETTER KHAMTI CA | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+AA62 | `ꩢ` | MYANMAR LETTER KHAMTI CHA | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+AA63 | `ꩣ` | MYANMAR LETTER KHAMTI JA | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+AA64 | `ꩤ` | MYANMAR LETTER KHAMTI JHA | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+AA65 | `ꩥ` | MYANMAR LETTER KHAMTI NYA | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+AA66 | `ꩦ` | MYANMAR LETTER KHAMTI TTA | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+AA67 | `ꩧ` | MYANMAR LETTER KHAMTI TTHA | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+AA68 | `ꩨ` | MYANMAR LETTER KHAMTI DDA | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+AA69 | `ꩩ` | MYANMAR LETTER KHAMTI DDHA | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+AA6A | `ꩪ` | MYANMAR LETTER KHAMTI DHA | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+AA6B | `ꩫ` | MYANMAR LETTER KHAMTI NA | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+AA6C | `ꩬ` | MYANMAR LETTER KHAMTI SA | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+AA6D | `ꩭ` | MYANMAR LETTER KHAMTI HA | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+AA6E | `ꩮ` | MYANMAR LETTER KHAMTI HHA | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+AA6F | `ꩯ` | MYANMAR LETTER KHAMTI FA | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+AA70 | `ꩰ` | MYANMAR MODIFIER LETTER KHAMTI REDUPLICATION | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+AA71 | `ꩱ` | MYANMAR LETTER KHAMTI XA | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+AA72 | `ꩲ` | MYANMAR LETTER KHAMTI ZA | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+AA73 | `ꩳ` | MYANMAR LETTER KHAMTI RA | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+AA74 | `ꩴ` | MYANMAR LOGOGRAM KHAMTI OAY | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+AA75 | `ꩵ` | MYANMAR LOGOGRAM KHAMTI QN | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+AA76 | `ꩶ` | MYANMAR LOGOGRAM KHAMTI HM | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+AA77 | `꩷` | MYANMAR SYMBOL AITON EXCLAMATION | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+AA78 | `꩸` | MYANMAR SYMBOL AITON ONE | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+AA79 | `꩹` | MYANMAR SYMBOL AITON TWO | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+AA7A | `ꩺ` | MYANMAR LETTER AITON RA | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+AA7B | `◌ꩻ` | MYANMAR SIGN PAO KAREN TONE | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+AA7C | `◌ꩼ` | MYANMAR SIGN TAI LAING TONE-2 | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+AA7D | `◌ꩽ` | MYANMAR SIGN TAI LAING TONE-5 | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+AA7E | `ꩾ` | MYANMAR LETTER SHWE PALAUNG CHA | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+AA7F | `ꩿ` | MYANMAR LETTER SHWE PALAUNG SHA | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+A9E0 | `ꧠ` | MYANMAR LETTER SHAN GHA | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+A9E1 | `ꧡ` | MYANMAR LETTER SHAN CHA | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+A9E2 | `ꧢ` | MYANMAR LETTER SHAN JHA | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+A9E3 | `ꧣ` | MYANMAR LETTER SHAN NNA | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+A9E4 | `ꧤ` | MYANMAR LETTER SHAN BHA | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+A9E5 | `◌ꧥ` | MYANMAR SIGN SHAN SAW | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+A9E6 | `ꧦ` | MYANMAR MODIFIER LETTER SHAN REDUPLICATION | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+A9E7 | `ꧧ` | MYANMAR LETTER TAI LAING NYA | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+A9E8 | `ꧨ` | MYANMAR LETTER TAI LAING FA | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+A9E9 | `ꧩ` | MYANMAR LETTER TAI LAING GA | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+A9EA | `ꧪ` | MYANMAR LETTER TAI LAING GHA | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+A9EB | `ꧫ` | MYANMAR LETTER TAI LAING JA | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+A9EC | `ꧬ` | MYANMAR LETTER TAI LAING JHA | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+A9ED | `ꧭ` | MYANMAR LETTER TAI LAING DDA | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+A9EE | `ꧮ` | MYANMAR LETTER TAI LAING DDHA | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+A9EF | `ꧯ` | MYANMAR LETTER TAI LAING NNA | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+A9F0 | `꧰` | MYANMAR TAI LAING DIGIT ZERO | Numeric value `0` | Add numeric conversion; check neighboring numerals and target representation. |
| U+A9F1 | `꧱` | MYANMAR TAI LAING DIGIT ONE | Numeric value `1` | Add numeric conversion; check neighboring numerals and target representation. |
| U+A9F2 | `꧲` | MYANMAR TAI LAING DIGIT TWO | Numeric value `2` | Add numeric conversion; check neighboring numerals and target representation. |
| U+A9F3 | `꧳` | MYANMAR TAI LAING DIGIT THREE | Numeric value `3` | Add numeric conversion; check neighboring numerals and target representation. |
| U+A9F4 | `꧴` | MYANMAR TAI LAING DIGIT FOUR | Numeric value `4` | Add numeric conversion; check neighboring numerals and target representation. |
| U+A9F5 | `꧵` | MYANMAR TAI LAING DIGIT FIVE | Numeric value `5` | Add numeric conversion; check neighboring numerals and target representation. |
| U+A9F6 | `꧶` | MYANMAR TAI LAING DIGIT SIX | Numeric value `6` | Add numeric conversion; check neighboring numerals and target representation. |
| U+A9F7 | `꧷` | MYANMAR TAI LAING DIGIT SEVEN | Numeric value `7` | Add numeric conversion; check neighboring numerals and target representation. |
| U+A9F8 | `꧸` | MYANMAR TAI LAING DIGIT EIGHT | Numeric value `8` | Add numeric conversion; check neighboring numerals and target representation. |
| U+A9F9 | `꧹` | MYANMAR TAI LAING DIGIT NINE | Numeric value `9` | Add numeric conversion; check neighboring numerals and target representation. |
| U+A9FA | `ꧺ` | MYANMAR LETTER TAI LAING LLA | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+A9FB | `ꧻ` | MYANMAR LETTER TAI LAING DA | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+A9FC | `ꧼ` | MYANMAR LETTER TAI LAING DHA | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+A9FD | `ꧽ` | MYANMAR LETTER TAI LAING BA | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+A9FE | `ꧾ` | MYANMAR LETTER TAI LAING BHA | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |

## Devanagari (38 candidates)

Blocks: U+0900–U+097F, U+A8E0–U+A8FF, U+11B00–U+11B5F.

| Code point | Character | Unicode name | Closest candidate | Possible Inditrans solution |
|---|---|---|---|---|
| U+0900 | `◌ऀ` | DEVANAGARI SIGN INVERTED CANDRABINDU | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+0904 | `ऄ` | DEVANAGARI LETTER SHORT A | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+093A | `◌ऺ` | DEVANAGARI VOWEL SIGN OE | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+093B | `◌ऻ` | DEVANAGARI VOWEL SIGN OOE | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+093C | `◌़` | DEVANAGARI SIGN NUKTA **★** | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+094E | `◌ॎ` | DEVANAGARI VOWEL SIGN PRISHTHAMATRA E | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+094F | `◌ॏ` | DEVANAGARI VOWEL SIGN AW | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+0953 | `◌॓` | DEVANAGARI GRAVE ACCENT | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+0954 | `◌॔` | DEVANAGARI ACUTE ACCENT | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+0956 | `◌ॖ` | DEVANAGARI VOWEL SIGN UE | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+0957 | `◌ॗ` | DEVANAGARI VOWEL SIGN UUE | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+0971 | `ॱ` | DEVANAGARI SIGN HIGH SPACING DOT | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+0973 | `ॳ` | DEVANAGARI LETTER OE | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+0974 | `ॴ` | DEVANAGARI LETTER OOE | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+0975 | `ॵ` | DEVANAGARI LETTER AW | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+0976 | `ॶ` | DEVANAGARI LETTER UE | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+0977 | `ॷ` | DEVANAGARI LETTER UUE | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+0978 | `ॸ` | DEVANAGARI LETTER MARWARI DDA | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+0979 | `ॹ` | DEVANAGARI LETTER ZHA | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+097A | `ॺ` | DEVANAGARI LETTER HEAVY YA | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+097B | `ॻ` | DEVANAGARI LETTER GGA | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+097C | `ॼ` | DEVANAGARI LETTER JJA | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+097D | `ॽ` | DEVANAGARI LETTER GLOTTAL STOP | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+097E | `ॾ` | DEVANAGARI LETTER DDDA | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+097F | `ॿ` | DEVANAGARI LETTER BBA | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+A8E0 | `◌꣠` | COMBINING DEVANAGARI DIGIT ZERO | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+A8FE | `ꣾ` | DEVANAGARI LETTER AY | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+A8FF | `◌ꣿ` | DEVANAGARI VOWEL SIGN AY | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+11B00 | `𑬀` | DEVANAGARI HEAD MARK | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+11B01 | `𑬁` | DEVANAGARI HEAD MARK WITH HEADSTROKE | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+11B02 | `𑬂` | DEVANAGARI SIGN BHALE | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+11B03 | `𑬃` | DEVANAGARI SIGN BHALE WITH HOOK | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+11B04 | `𑬄` | DEVANAGARI SIGN EXTENDED BHALE | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+11B05 | `𑬅` | DEVANAGARI SIGN EXTENDED BHALE WITH HOOK | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+11B06 | `𑬆` | DEVANAGARI SIGN WESTERN FIVE-LIKE BHALE | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+11B07 | `𑬇` | DEVANAGARI SIGN WESTERN NINE-LIKE BHALE | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+11B08 | `𑬈` | DEVANAGARI SIGN REVERSED NINE-LIKE BHALE | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+11B09 | `𑬉` | DEVANAGARI SIGN MINDU | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |

## Grantha (17 candidates)

Blocks: U+11300–U+1137F.

| Code point | Character | Unicode name | Closest candidate | Possible Inditrans solution |
|---|---|---|---|---|
| U+11300 | `◌𑌀` | GRANTHA SIGN COMBINING ANUSVARA ABOVE | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+1133B | `◌𑌻` | COMBINING BINDU BELOW | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+1133C | `◌𑌼` | GRANTHA SIGN NUKTA | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+11357 | `◌𑍗` | GRANTHA AU LENGTH MARK | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+1135D | `𑍝` | GRANTHA SIGN PLUTA | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+11366 | `◌𑍦` | COMBINING GRANTHA DIGIT ZERO | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+11367 | `◌𑍧` | COMBINING GRANTHA DIGIT ONE | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+11368 | `◌𑍨` | COMBINING GRANTHA DIGIT TWO | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+11369 | `◌𑍩` | COMBINING GRANTHA DIGIT THREE | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+1136A | `◌𑍪` | COMBINING GRANTHA DIGIT FOUR | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+1136B | `◌𑍫` | COMBINING GRANTHA DIGIT FIVE | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+1136C | `◌𑍬` | COMBINING GRANTHA DIGIT SIX | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+11370 | `◌𑍰` | COMBINING GRANTHA LETTER A | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+11371 | `◌𑍱` | COMBINING GRANTHA LETTER KA | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+11372 | `◌𑍲` | COMBINING GRANTHA LETTER NA | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+11373 | `◌𑍳` | COMBINING GRANTHA LETTER VI | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+11374 | `◌𑍴` | COMBINING GRANTHA LETTER PA | Not determined | Review mark function; add a diacritic alias or contextual handling. |

## Gujarati (9 candidates)

Blocks: U+0A80–U+0AFF.

| Code point | Character | Unicode name | Closest candidate | Possible Inditrans solution |
|---|---|---|---|---|
| U+0ABC | `◌઼` | GUJARATI SIGN NUKTA | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+0AF1 | `૱` | GUJARATI RUPEE SIGN | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+0AF9 | `ૹ` | GUJARATI LETTER ZHA | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+0AFA | `◌ૺ` | GUJARATI SIGN SUKUN | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+0AFB | `◌ૻ` | GUJARATI SIGN SHADDA | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+0AFC | `◌ૼ` | GUJARATI SIGN MADDAH | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+0AFD | `◌૽` | GUJARATI SIGN THREE-DOT NUKTA ABOVE | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+0AFE | `◌૾` | GUJARATI SIGN CIRCLE NUKTA ABOVE | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+0AFF | `◌૿` | GUJARATI SIGN TWO-CIRCLE NUKTA ABOVE | Not determined | Review mark function; add a diacritic alias or contextual handling. |

## Gurmukhi (3 candidates)

Blocks: U+0A00–U+0A7F.

| Code point | Character | Unicode name | Closest candidate | Possible Inditrans solution |
|---|---|---|---|---|
| U+0A3C | `◌਼` | GURMUKHI SIGN NUKTA | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+0A51 | `◌ੑ` | GURMUKHI SIGN UDAAT | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+0A75 | `◌ੵ` | GURMUKHI SIGN YAKASH | Not determined | Review mark function; add a diacritic alias or contextual handling. |

## Kannada (6 candidates)

Blocks: U+0C80–U+0CFF.

| Code point | Character | Unicode name | Closest candidate | Possible Inditrans solution |
|---|---|---|---|---|
| U+0C80 | `ಀ` | KANNADA SIGN SPACING CANDRABINDU | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+0C84 | `಄` | KANNADA SIGN SIDDHAM | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+0CBC | `◌಼` | KANNADA SIGN NUKTA | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+0CD5 | `◌ೕ` | KANNADA LENGTH MARK | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+0CD6 | `◌ೖ` | KANNADA AI LENGTH MARK | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+0CF3 | `◌ೳ` | KANNADA SIGN COMBINING ANUSVARA ABOVE RIGHT | Not determined | Review mark function; add a diacritic alias or contextual handling. |

## Khmer (58 candidates)

Blocks: U+1780–U+17FF, U+19E0–U+19FF.

| Code point | Character | Unicode name | Closest candidate | Possible Inditrans solution |
|---|---|---|---|---|
| U+17B4 | `◌឴` | KHMER VOWEL INHERENT AQ | Inherent vowel | Allow a documented no-op without suppressing the preceding vowel. |
| U+17C8 | `◌ៈ` | KHMER SIGN YUUKALEAPINTU **★** | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+17C9 | `◌៉` | KHMER SIGN MUUSIKATOAN **★** | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+17CA | `◌៊` | KHMER SIGN TRIISAP **★** | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+17CB | `◌់` | KHMER SIGN BANTOC **★** | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+17CC | `◌៌` | KHMER SIGN ROBAT | `r` | Add contextual repha-style handling; verify register and position. |
| U+17CD | `◌៍` | KHMER SIGN TOANDAKHIAT **★** | Silent-letter marker | Add contextual suppression or document a lossy omission. |
| U+17CE | `◌៎` | KHMER SIGN KAKABAT | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+17CF | `◌៏` | KHMER SIGN AHSDA | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+17D0 | `◌័` | KHMER SIGN SAMYOK SANNYA **★** | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+17D3 | `◌៓` | KHMER SIGN BATHAMASAT | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+17D7 | `ៗ` | KHMER SIGN LEK TOO **★** | Previous word or phrase | Add contextual repetition handling; no fixed alias. |
| U+17D8 | `៘` | KHMER SIGN BEYYAL | `។ល។` (et cetera) | Expand to existing punctuation and letter tokens; verify target conventions. |
| U+17D9 | `៙` | KHMER SIGN PHNAEK MUAN **★** | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+17DB | `៛` | KHMER CURRENCY SYMBOL RIEL | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+17DD | `◌៝` | KHMER SIGN ATTHACAN | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+17F0 | `៰` | KHMER SYMBOL LEK ATTAK SON | Numeric value `0` | Add numeric conversion; check neighboring numerals and target representation. |
| U+17F1 | `៱` | KHMER SYMBOL LEK ATTAK MUOY | Numeric value `1` | Add numeric conversion; check neighboring numerals and target representation. |
| U+17F2 | `៲` | KHMER SYMBOL LEK ATTAK PII | Numeric value `2` | Add numeric conversion; check neighboring numerals and target representation. |
| U+17F3 | `៳` | KHMER SYMBOL LEK ATTAK BEI | Numeric value `3` | Add numeric conversion; check neighboring numerals and target representation. |
| U+17F4 | `៴` | KHMER SYMBOL LEK ATTAK BUON | Numeric value `4` | Add numeric conversion; check neighboring numerals and target representation. |
| U+17F5 | `៵` | KHMER SYMBOL LEK ATTAK PRAM | Numeric value `5` | Add numeric conversion; check neighboring numerals and target representation. |
| U+17F6 | `៶` | KHMER SYMBOL LEK ATTAK PRAM-MUOY | Numeric value `6` | Add numeric conversion; check neighboring numerals and target representation. |
| U+17F7 | `៷` | KHMER SYMBOL LEK ATTAK PRAM-PII | Numeric value `7` | Add numeric conversion; check neighboring numerals and target representation. |
| U+17F8 | `៸` | KHMER SYMBOL LEK ATTAK PRAM-BEI | Numeric value `8` | Add numeric conversion; check neighboring numerals and target representation. |
| U+17F9 | `៹` | KHMER SYMBOL LEK ATTAK PRAM-BUON | Numeric value `9` | Add numeric conversion; check neighboring numerals and target representation. |
| U+19E0 | `᧠` | KHMER SYMBOL PATHAMASAT | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+19E1 | `᧡` | KHMER SYMBOL MUOY KOET | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+19E2 | `᧢` | KHMER SYMBOL PII KOET | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+19E3 | `᧣` | KHMER SYMBOL BEI KOET | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+19E4 | `᧤` | KHMER SYMBOL BUON KOET | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+19E5 | `᧥` | KHMER SYMBOL PRAM KOET | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+19E6 | `᧦` | KHMER SYMBOL PRAM-MUOY KOET | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+19E7 | `᧧` | KHMER SYMBOL PRAM-PII KOET | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+19E8 | `᧨` | KHMER SYMBOL PRAM-BEI KOET | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+19E9 | `᧩` | KHMER SYMBOL PRAM-BUON KOET | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+19EA | `᧪` | KHMER SYMBOL DAP KOET | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+19EB | `᧫` | KHMER SYMBOL DAP-MUOY KOET | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+19EC | `᧬` | KHMER SYMBOL DAP-PII KOET | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+19ED | `᧭` | KHMER SYMBOL DAP-BEI KOET | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+19EE | `᧮` | KHMER SYMBOL DAP-BUON KOET | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+19EF | `᧯` | KHMER SYMBOL DAP-PRAM KOET | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+19F0 | `᧰` | KHMER SYMBOL TUTEYASAT | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+19F1 | `᧱` | KHMER SYMBOL MUOY ROC | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+19F2 | `᧲` | KHMER SYMBOL PII ROC | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+19F3 | `᧳` | KHMER SYMBOL BEI ROC | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+19F4 | `᧴` | KHMER SYMBOL BUON ROC | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+19F5 | `᧵` | KHMER SYMBOL PRAM ROC | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+19F6 | `᧶` | KHMER SYMBOL PRAM-MUOY ROC | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+19F7 | `᧷` | KHMER SYMBOL PRAM-PII ROC | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+19F8 | `᧸` | KHMER SYMBOL PRAM-BEI ROC | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+19F9 | `᧹` | KHMER SYMBOL PRAM-BUON ROC | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+19FA | `᧺` | KHMER SYMBOL DAP ROC | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+19FB | `᧻` | KHMER SYMBOL DAP-MUOY ROC | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+19FC | `᧼` | KHMER SYMBOL DAP-PII ROC | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+19FD | `᧽` | KHMER SYMBOL DAP-BEI ROC | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+19FE | `᧾` | KHMER SYMBOL DAP-BUON ROC | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+19FF | `᧿` | KHMER SYMBOL DAP-PRAM ROC | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |

## Malayalam (24 candidates)

Blocks: U+0D00–U+0D7F.

| Code point | Character | Unicode name | Closest candidate | Possible Inditrans solution |
|---|---|---|---|---|
| U+0D00 | `◌ഀ` | MALAYALAM SIGN COMBINING ANUSVARA ABOVE | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+0D04 | `ഄ` | MALAYALAM LETTER VEDIC ANUSVARA | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+0D3B | `◌഻` | MALAYALAM SIGN VERTICAL BAR VIRAMA | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+0D3C | `◌഼` | MALAYALAM SIGN CIRCULAR VIRAMA | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+0D4E | `ൎ` | MALAYALAM LETTER DOT REPH | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+0D4F | `൏` | MALAYALAM SIGN PARA | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+0D58 | `൘` | MALAYALAM FRACTION ONE ONE-HUNDRED-AND-SIXTIETH | Numeric value `1/160` | Add numeric conversion; check neighboring numerals and target representation. |
| U+0D59 | `൙` | MALAYALAM FRACTION ONE FORTIETH | Numeric value `1/40` | Add numeric conversion; check neighboring numerals and target representation. |
| U+0D5A | `൚` | MALAYALAM FRACTION THREE EIGHTIETHS | Numeric value `3/80` | Add numeric conversion; check neighboring numerals and target representation. |
| U+0D5B | `൛` | MALAYALAM FRACTION ONE TWENTIETH | Numeric value `1/20` | Add numeric conversion; check neighboring numerals and target representation. |
| U+0D5C | `൜` | MALAYALAM FRACTION ONE TENTH | Numeric value `1/10` | Add numeric conversion; check neighboring numerals and target representation. |
| U+0D5D | `൝` | MALAYALAM FRACTION THREE TWENTIETHS | Numeric value `3/20` | Add numeric conversion; check neighboring numerals and target representation. |
| U+0D5E | `൞` | MALAYALAM FRACTION ONE FIFTH | Numeric value `1/5` | Add numeric conversion; check neighboring numerals and target representation. |
| U+0D5F | `ൟ` | MALAYALAM LETTER ARCHAIC II | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+0D70 | `൰` | MALAYALAM NUMBER TEN | Numeric value `10` | Add numeric conversion; check neighboring numerals and target representation. |
| U+0D71 | `൱` | MALAYALAM NUMBER ONE HUNDRED | Numeric value `100` | Add numeric conversion; check neighboring numerals and target representation. |
| U+0D72 | `൲` | MALAYALAM NUMBER ONE THOUSAND | Numeric value `1000` | Add numeric conversion; check neighboring numerals and target representation. |
| U+0D73 | `൳` | MALAYALAM FRACTION ONE QUARTER | Numeric value `1/4` | Add numeric conversion; check neighboring numerals and target representation. |
| U+0D74 | `൴` | MALAYALAM FRACTION ONE HALF | Numeric value `1/2` | Add numeric conversion; check neighboring numerals and target representation. |
| U+0D75 | `൵` | MALAYALAM FRACTION THREE QUARTERS | Numeric value `3/4` | Add numeric conversion; check neighboring numerals and target representation. |
| U+0D76 | `൶` | MALAYALAM FRACTION ONE SIXTEENTH | Numeric value `1/16` | Add numeric conversion; check neighboring numerals and target representation. |
| U+0D77 | `൷` | MALAYALAM FRACTION ONE EIGHTH | Numeric value `1/8` | Add numeric conversion; check neighboring numerals and target representation. |
| U+0D78 | `൸` | MALAYALAM FRACTION THREE SIXTEENTHS | Numeric value `3/16` | Add numeric conversion; check neighboring numerals and target representation. |
| U+0D79 | `൹` | MALAYALAM DATE MARK | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |

## Oriya (11 candidates)

Blocks: U+0B00–U+0B7F.

| Code point | Character | Unicode name | Closest candidate | Possible Inditrans solution |
|---|---|---|---|---|
| U+0B3C | `◌଼` | ORIYA SIGN NUKTA **★** | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+0B55 | `◌୕` | ORIYA SIGN OVERLINE | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+0B56 | `◌ୖ` | ORIYA AI LENGTH MARK | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+0B57 | `◌ୗ` | ORIYA AU LENGTH MARK | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+0B70 | `୰` | ORIYA ISSHAR | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+0B72 | `୲` | ORIYA FRACTION ONE QUARTER | Numeric value `1/4` | Add numeric conversion; check neighboring numerals and target representation. |
| U+0B73 | `୳` | ORIYA FRACTION ONE HALF | Numeric value `1/2` | Add numeric conversion; check neighboring numerals and target representation. |
| U+0B74 | `୴` | ORIYA FRACTION THREE QUARTERS | Numeric value `3/4` | Add numeric conversion; check neighboring numerals and target representation. |
| U+0B75 | `୵` | ORIYA FRACTION ONE SIXTEENTH | Numeric value `1/16` | Add numeric conversion; check neighboring numerals and target representation. |
| U+0B76 | `୶` | ORIYA FRACTION ONE EIGHTH | Numeric value `1/8` | Add numeric conversion; check neighboring numerals and target representation. |
| U+0B77 | `୷` | ORIYA FRACTION THREE SIXTEENTHS | Numeric value `3/16` | Add numeric conversion; check neighboring numerals and target representation. |

## Sinhala (22 candidates)

Blocks: U+0D80–U+0DFF, U+111E0–U+111FF.

| Code point | Character | Unicode name | Closest candidate | Possible Inditrans solution |
|---|---|---|---|---|
| U+0D81 | `◌ඁ` | SINHALA SIGN CANDRABINDU | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+0DA5 | `ඥ` | SINHALA LETTER TAALUJA SANYOOGA NAAKSIKYAYA **★** | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+111E1 | `𑇡` | SINHALA ARCHAIC DIGIT ONE | Numeric value `1` | Add numeric conversion; check neighboring numerals and target representation. |
| U+111E2 | `𑇢` | SINHALA ARCHAIC DIGIT TWO | Numeric value `2` | Add numeric conversion; check neighboring numerals and target representation. |
| U+111E3 | `𑇣` | SINHALA ARCHAIC DIGIT THREE | Numeric value `3` | Add numeric conversion; check neighboring numerals and target representation. |
| U+111E4 | `𑇤` | SINHALA ARCHAIC DIGIT FOUR | Numeric value `4` | Add numeric conversion; check neighboring numerals and target representation. |
| U+111E5 | `𑇥` | SINHALA ARCHAIC DIGIT FIVE | Numeric value `5` | Add numeric conversion; check neighboring numerals and target representation. |
| U+111E6 | `𑇦` | SINHALA ARCHAIC DIGIT SIX | Numeric value `6` | Add numeric conversion; check neighboring numerals and target representation. |
| U+111E7 | `𑇧` | SINHALA ARCHAIC DIGIT SEVEN | Numeric value `7` | Add numeric conversion; check neighboring numerals and target representation. |
| U+111E8 | `𑇨` | SINHALA ARCHAIC DIGIT EIGHT | Numeric value `8` | Add numeric conversion; check neighboring numerals and target representation. |
| U+111E9 | `𑇩` | SINHALA ARCHAIC DIGIT NINE | Numeric value `9` | Add numeric conversion; check neighboring numerals and target representation. |
| U+111EA | `𑇪` | SINHALA ARCHAIC NUMBER TEN | Numeric value `10` | Add numeric conversion; check neighboring numerals and target representation. |
| U+111EB | `𑇫` | SINHALA ARCHAIC NUMBER TWENTY | Numeric value `20` | Add numeric conversion; check neighboring numerals and target representation. |
| U+111EC | `𑇬` | SINHALA ARCHAIC NUMBER THIRTY | Numeric value `30` | Add numeric conversion; check neighboring numerals and target representation. |
| U+111ED | `𑇭` | SINHALA ARCHAIC NUMBER FORTY | Numeric value `40` | Add numeric conversion; check neighboring numerals and target representation. |
| U+111EE | `𑇮` | SINHALA ARCHAIC NUMBER FIFTY | Numeric value `50` | Add numeric conversion; check neighboring numerals and target representation. |
| U+111EF | `𑇯` | SINHALA ARCHAIC NUMBER SIXTY | Numeric value `60` | Add numeric conversion; check neighboring numerals and target representation. |
| U+111F0 | `𑇰` | SINHALA ARCHAIC NUMBER SEVENTY | Numeric value `70` | Add numeric conversion; check neighboring numerals and target representation. |
| U+111F1 | `𑇱` | SINHALA ARCHAIC NUMBER EIGHTY | Numeric value `80` | Add numeric conversion; check neighboring numerals and target representation. |
| U+111F2 | `𑇲` | SINHALA ARCHAIC NUMBER NINETY | Numeric value `90` | Add numeric conversion; check neighboring numerals and target representation. |
| U+111F3 | `𑇳` | SINHALA ARCHAIC NUMBER ONE HUNDRED | Numeric value `100` | Add numeric conversion; check neighboring numerals and target representation. |
| U+111F4 | `𑇴` | SINHALA ARCHAIC NUMBER ONE THOUSAND | Numeric value `1000` | Add numeric conversion; check neighboring numerals and target representation. |

## Tamil (63 candidates)

Blocks: U+0B80–U+0BFF, U+11FC0–U+11FFF.

| Code point | Character | Unicode name | Closest candidate | Possible Inditrans solution |
|---|---|---|---|---|
| U+0BD7 | `◌ௗ` | TAMIL AU LENGTH MARK | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+0BF0 | `௰` | TAMIL NUMBER TEN | Numeric value `10` | Add numeric conversion; check neighboring numerals and target representation. |
| U+0BF1 | `௱` | TAMIL NUMBER ONE HUNDRED | Numeric value `100` | Add numeric conversion; check neighboring numerals and target representation. |
| U+0BF2 | `௲` | TAMIL NUMBER ONE THOUSAND | Numeric value `1000` | Add numeric conversion; check neighboring numerals and target representation. |
| U+0BF3 | `௳` | TAMIL DAY SIGN | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+0BF4 | `௴` | TAMIL MONTH SIGN | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+0BF5 | `௵` | TAMIL YEAR SIGN | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+0BF6 | `௶` | TAMIL DEBIT SIGN | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+0BF7 | `௷` | TAMIL CREDIT SIGN | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+0BF8 | `௸` | TAMIL AS ABOVE SIGN | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+0BF9 | `௹` | TAMIL RUPEE SIGN | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+0BFA | `௺` | TAMIL NUMBER SIGN | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+11FC0 | `𑿀` | TAMIL FRACTION ONE THREE-HUNDRED-AND-TWENTIETH | Numeric value `1/320` | Add numeric conversion; check neighboring numerals and target representation. |
| U+11FC1 | `𑿁` | TAMIL FRACTION ONE ONE-HUNDRED-AND-SIXTIETH | Numeric value `1/160` | Add numeric conversion; check neighboring numerals and target representation. |
| U+11FC2 | `𑿂` | TAMIL FRACTION ONE EIGHTIETH | Numeric value `1/80` | Add numeric conversion; check neighboring numerals and target representation. |
| U+11FC3 | `𑿃` | TAMIL FRACTION ONE SIXTY-FOURTH | Numeric value `1/64` | Add numeric conversion; check neighboring numerals and target representation. |
| U+11FC4 | `𑿄` | TAMIL FRACTION ONE FORTIETH | Numeric value `1/40` | Add numeric conversion; check neighboring numerals and target representation. |
| U+11FC5 | `𑿅` | TAMIL FRACTION ONE THIRTY-SECOND | Numeric value `1/32` | Add numeric conversion; check neighboring numerals and target representation. |
| U+11FC6 | `𑿆` | TAMIL FRACTION THREE EIGHTIETHS | Numeric value `3/80` | Add numeric conversion; check neighboring numerals and target representation. |
| U+11FC7 | `𑿇` | TAMIL FRACTION THREE SIXTY-FOURTHS | Numeric value `3/64` | Add numeric conversion; check neighboring numerals and target representation. |
| U+11FC8 | `𑿈` | TAMIL FRACTION ONE TWENTIETH | Numeric value `1/20` | Add numeric conversion; check neighboring numerals and target representation. |
| U+11FC9 | `𑿉` | TAMIL FRACTION ONE SIXTEENTH-1 | Numeric value `1/16` | Add numeric conversion; check neighboring numerals and target representation. |
| U+11FCA | `𑿊` | TAMIL FRACTION ONE SIXTEENTH-2 | Numeric value `1/16` | Add numeric conversion; check neighboring numerals and target representation. |
| U+11FCB | `𑿋` | TAMIL FRACTION ONE TENTH | Numeric value `1/10` | Add numeric conversion; check neighboring numerals and target representation. |
| U+11FCC | `𑿌` | TAMIL FRACTION ONE EIGHTH | Numeric value `1/8` | Add numeric conversion; check neighboring numerals and target representation. |
| U+11FCD | `𑿍` | TAMIL FRACTION THREE TWENTIETHS | Numeric value `3/20` | Add numeric conversion; check neighboring numerals and target representation. |
| U+11FCE | `𑿎` | TAMIL FRACTION THREE SIXTEENTHS | Numeric value `3/16` | Add numeric conversion; check neighboring numerals and target representation. |
| U+11FCF | `𑿏` | TAMIL FRACTION ONE FIFTH | Numeric value `1/5` | Add numeric conversion; check neighboring numerals and target representation. |
| U+11FD0 | `𑿐` | TAMIL FRACTION ONE QUARTER | Numeric value `1/4` | Add numeric conversion; check neighboring numerals and target representation. |
| U+11FD1 | `𑿑` | TAMIL FRACTION ONE HALF-1 | Numeric value `1/2` | Add numeric conversion; check neighboring numerals and target representation. |
| U+11FD2 | `𑿒` | TAMIL FRACTION ONE HALF-2 | Numeric value `1/2` | Add numeric conversion; check neighboring numerals and target representation. |
| U+11FD3 | `𑿓` | TAMIL FRACTION THREE QUARTERS | Numeric value `3/4` | Add numeric conversion; check neighboring numerals and target representation. |
| U+11FD4 | `𑿔` | TAMIL FRACTION DOWNSCALING FACTOR KIIZH | Numeric value `1/320` | Add numeric conversion; check neighboring numerals and target representation. |
| U+11FD5 | `𑿕` | TAMIL SIGN NEL | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+11FD6 | `𑿖` | TAMIL SIGN CEVITU | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+11FD7 | `𑿗` | TAMIL SIGN AAZHAAKKU | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+11FD8 | `𑿘` | TAMIL SIGN UZHAKKU | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+11FD9 | `𑿙` | TAMIL SIGN MUUVUZHAKKU | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+11FDA | `𑿚` | TAMIL SIGN KURUNI | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+11FDB | `𑿛` | TAMIL SIGN PATHAKKU | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+11FDC | `𑿜` | TAMIL SIGN MUKKURUNI | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+11FDD | `𑿝` | TAMIL SIGN KAACU | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+11FDE | `𑿞` | TAMIL SIGN PANAM | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+11FDF | `𑿟` | TAMIL SIGN PON | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+11FE0 | `𑿠` | TAMIL SIGN VARAAKAN | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+11FE1 | `𑿡` | TAMIL SIGN PAARAM | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+11FE2 | `𑿢` | TAMIL SIGN KUZHI | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+11FE3 | `𑿣` | TAMIL SIGN VELI | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+11FE4 | `𑿤` | TAMIL WET CULTIVATION SIGN | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+11FE5 | `𑿥` | TAMIL DRY CULTIVATION SIGN | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+11FE6 | `𑿦` | TAMIL LAND SIGN | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+11FE7 | `𑿧` | TAMIL SALT PAN SIGN | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+11FE8 | `𑿨` | TAMIL TRADITIONAL CREDIT SIGN | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+11FE9 | `𑿩` | TAMIL TRADITIONAL NUMBER SIGN | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+11FEA | `𑿪` | TAMIL CURRENT SIGN | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+11FEB | `𑿫` | TAMIL AND ODD SIGN | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+11FEC | `𑿬` | TAMIL SPENT SIGN | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+11FED | `𑿭` | TAMIL TOTAL SIGN | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+11FEE | `𑿮` | TAMIL IN POSSESSION SIGN | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+11FEF | `𑿯` | TAMIL STARTING FROM SIGN | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+11FF0 | `𑿰` | TAMIL SIGN MUTHALIYA | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+11FF1 | `𑿱` | TAMIL SIGN VAKAIYARAA | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+11FFF | `𑿿` | TAMIL PUNCTUATION END OF TEXT | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |

## Telugu (13 candidates)

Blocks: U+0C00–U+0C7F.

| Code point | Character | Unicode name | Closest candidate | Possible Inditrans solution |
|---|---|---|---|---|
| U+0C04 | `◌ఄ` | TELUGU SIGN COMBINING ANUSVARA ABOVE | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+0C3C | `◌఼` | TELUGU SIGN NUKTA | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+0C55 | `◌ౕ` | TELUGU LENGTH MARK | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+0C56 | `◌ౖ` | TELUGU AI LENGTH MARK | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+0C77 | `౷` | TELUGU SIGN SIDDHAM | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+0C78 | `౸` | TELUGU FRACTION DIGIT ZERO FOR ODD POWERS OF FOUR | Numeric value `0` | Add numeric conversion; check neighboring numerals and target representation. |
| U+0C79 | `౹` | TELUGU FRACTION DIGIT ONE FOR ODD POWERS OF FOUR | Numeric value `1` | Add numeric conversion; check neighboring numerals and target representation. |
| U+0C7A | `౺` | TELUGU FRACTION DIGIT TWO FOR ODD POWERS OF FOUR | Numeric value `2` | Add numeric conversion; check neighboring numerals and target representation. |
| U+0C7B | `౻` | TELUGU FRACTION DIGIT THREE FOR ODD POWERS OF FOUR | Numeric value `3` | Add numeric conversion; check neighboring numerals and target representation. |
| U+0C7C | `౼` | TELUGU FRACTION DIGIT ONE FOR EVEN POWERS OF FOUR | Numeric value `1` | Add numeric conversion; check neighboring numerals and target representation. |
| U+0C7D | `౽` | TELUGU FRACTION DIGIT TWO FOR EVEN POWERS OF FOUR | Numeric value `2` | Add numeric conversion; check neighboring numerals and target representation. |
| U+0C7E | `౾` | TELUGU FRACTION DIGIT THREE FOR EVEN POWERS OF FOUR | Numeric value `3` | Add numeric conversion; check neighboring numerals and target representation. |
| U+0C7F | `౿` | TELUGU SIGN TUUMU | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |

## Thai (13 candidates)

Blocks: U+0E00–U+0E7F.

| Code point | Character | Unicode name | Closest candidate | Possible Inditrans solution |
|---|---|---|---|---|
| U+0E31 | `◌ั` | THAI CHARACTER MAI HAN-AKAT **★** | Short a | Add a no-extra-vowel reading; preserve following consonants. |
| U+0E33 | `ำ` | THAI CHARACTER SARA AM **★** | Long a + nasal | Expand to vowel and anusvara tokens; verify Thai-specific behavior. |
| U+0E3F | `฿` | THAI CURRENCY SYMBOL BAHT | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+0E45 | `ๅ` | THAI CHARACTER LAKKHANGYAO **★** | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+0E46 | `ๆ` | THAI CHARACTER MAIYAMOK **★** | Previous word or phrase | Add contextual repetition handling; no fixed alias. |
| U+0E47 | `◌็` | THAI CHARACTER MAITAIKHU **★** | Shortening of preceding vowel | Add context-sensitive vowel handling or document a lossy omission. |
| U+0E48 | `◌่` | THAI CHARACTER MAI EK **★** | Tone mark; no matching token | Choose a documented tone approximation or omit it; avoid treating it as a vowel. |
| U+0E49 | `◌้` | THAI CHARACTER MAI THO **★** | Tone mark; no matching token | Choose a documented tone approximation or omit it; avoid treating it as a vowel. |
| U+0E4A | `◌๊` | THAI CHARACTER MAI TRI **★** | Tone mark; no matching token | Choose a documented tone approximation or omit it; avoid treating it as a vowel. |
| U+0E4B | `◌๋` | THAI CHARACTER MAI CHATTAWA **★** | Tone mark; no matching token | Choose a documented tone approximation or omit it; avoid treating it as a vowel. |
| U+0E4C | `◌์` | THAI CHARACTER THANTHAKHAT **★** | Suppress the marked consonant | Add contextual silent-consonant handling; a fixed alias cannot remove preceding output. |
| U+0E4E | `◌๎` | THAI CHARACTER YAMAKKAN **★** | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+0E4F | `๏` | THAI CHARACTER FONGMAN | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |

## Tibetan (96 candidates)

Blocks: U+0F00–U+0FFF.

| Code point | Character | Unicode name | Closest candidate | Possible Inditrans solution |
|---|---|---|---|---|
| U+0F01 | `༁` | TIBETAN MARK GTER YIG MGO TRUNCATED A | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+0F02 | `༂` | TIBETAN MARK GTER YIG MGO -UM RNAM BCAD MA | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+0F03 | `༃` | TIBETAN MARK GTER YIG MGO -UM GTER TSHEG MA | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+0F04 | `༄` | TIBETAN MARK INITIAL YIG MGO MDUN MA | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+0F05 | `༅` | TIBETAN MARK CLOSING YIG MGO SGAB MA | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+0F06 | `༆` | TIBETAN MARK CARET YIG MGO PHUR SHAD MA | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+0F07 | `༇` | TIBETAN MARK YIG MGO TSHEG SHAD MA | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+0F08 | `༈` | TIBETAN MARK SBRUL SHAD | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+0F09 | `༉` | TIBETAN MARK BSKUR YIG MGO | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+0F0A | `༊` | TIBETAN MARK BKA- SHOG YIG MGO | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+0F13 | `༓` | TIBETAN MARK CARET -DZUD RTAGS ME LONG CAN | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+0F15 | `༕` | TIBETAN LOGOTYPE SIGN CHAD RTAGS | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+0F16 | `༖` | TIBETAN LOGOTYPE SIGN LHAG RTAGS | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+0F17 | `༗` | TIBETAN ASTROLOGICAL SIGN SGRA GCAN -CHAR RTAGS | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+0F18 | `◌༘` | TIBETAN ASTROLOGICAL SIGN -KHYUD PA | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+0F19 | `◌༙` | TIBETAN ASTROLOGICAL SIGN SDONG TSHUGS | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+0F1A | `༚` | TIBETAN SIGN RDEL DKAR GCIG | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+0F1B | `༛` | TIBETAN SIGN RDEL DKAR GNYIS | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+0F1C | `༜` | TIBETAN SIGN RDEL DKAR GSUM | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+0F1D | `༝` | TIBETAN SIGN RDEL NAG GCIG | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+0F1E | `༞` | TIBETAN SIGN RDEL NAG GNYIS | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+0F1F | `༟` | TIBETAN SIGN RDEL DKAR RDEL NAG | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+0F2A | `༪` | TIBETAN DIGIT HALF ONE | Numeric value `1/2` | Add numeric conversion; check neighboring numerals and target representation. |
| U+0F2B | `༫` | TIBETAN DIGIT HALF TWO | Numeric value `3/2` | Add numeric conversion; check neighboring numerals and target representation. |
| U+0F2C | `༬` | TIBETAN DIGIT HALF THREE | Numeric value `5/2` | Add numeric conversion; check neighboring numerals and target representation. |
| U+0F2D | `༭` | TIBETAN DIGIT HALF FOUR | Numeric value `7/2` | Add numeric conversion; check neighboring numerals and target representation. |
| U+0F2E | `༮` | TIBETAN DIGIT HALF FIVE | Numeric value `9/2` | Add numeric conversion; check neighboring numerals and target representation. |
| U+0F2F | `༯` | TIBETAN DIGIT HALF SIX | Numeric value `11/2` | Add numeric conversion; check neighboring numerals and target representation. |
| U+0F30 | `༰` | TIBETAN DIGIT HALF SEVEN | Numeric value `13/2` | Add numeric conversion; check neighboring numerals and target representation. |
| U+0F31 | `༱` | TIBETAN DIGIT HALF EIGHT | Numeric value `15/2` | Add numeric conversion; check neighboring numerals and target representation. |
| U+0F32 | `༲` | TIBETAN DIGIT HALF NINE | Numeric value `17/2` | Add numeric conversion; check neighboring numerals and target representation. |
| U+0F33 | `༳` | TIBETAN DIGIT HALF ZERO | Numeric value `-1/2` | Add numeric conversion; check neighboring numerals and target representation. |
| U+0F34 | `༴` | TIBETAN MARK BSDUS RTAGS | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+0F35 | `◌༵` | TIBETAN MARK NGAS BZUNG NYI ZLA | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+0F36 | `༶` | TIBETAN MARK CARET -DZUD RTAGS BZHI MIG CAN | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+0F37 | `◌༷` | TIBETAN MARK NGAS BZUNG SGOR RTAGS | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+0F38 | `༸` | TIBETAN MARK CHE MGO | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+0F39 | `◌༹` | TIBETAN MARK TSA -PHRU | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+0F3A | `༺` | TIBETAN MARK GUG RTAGS GYON | `(` or `[` | Add punctuation token with a target fallback. |
| U+0F3B | `༻` | TIBETAN MARK GUG RTAGS GYAS | `)` or `]` | Add punctuation token with a target fallback. |
| U+0F3C | `༼` | TIBETAN MARK ANG KHANG GYON | `(` or `[` | Add punctuation token with a target fallback. |
| U+0F3D | `༽` | TIBETAN MARK ANG KHANG GYAS | `)` or `]` | Add punctuation token with a target fallback. |
| U+0F3E | `◌༾` | TIBETAN SIGN YAR TSHES | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+0F3F | `◌༿` | TIBETAN SIGN MAR TSHES | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+0F5C | `ཛྷ` | TIBETAN LETTER DZHA | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+0F60 | `འ` | TIBETAN LETTER -A | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+0F69 | `ཀྵ` | TIBETAN LETTER KSSA | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+0F73 | `◌ཱི` | TIBETAN VOWEL SIGN II | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+0F75 | `◌ཱུ` | TIBETAN VOWEL SIGN UU | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+0F76 | `◌ྲྀ` | TIBETAN VOWEL SIGN VOCALIC R | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+0F77 | `◌ཷ` | TIBETAN VOWEL SIGN VOCALIC RR **★** | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+0F78 | `◌ླྀ` | TIBETAN VOWEL SIGN VOCALIC L | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+0F79 | `◌ཹ` | TIBETAN VOWEL SIGN VOCALIC LL **★** | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+0F80 | `◌ྀ` | TIBETAN VOWEL SIGN REVERSED I **★** | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+0F81 | `◌ཱྀ` | TIBETAN VOWEL SIGN REVERSED II | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+0F82 | `◌ྂ` | TIBETAN SIGN NYI ZLA NAA DA | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+0F86 | `◌྆` | TIBETAN SIGN LCI RTAGS | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+0F87 | `◌྇` | TIBETAN SIGN YANG RTAGS | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+0F8A | `ྊ` | TIBETAN SIGN GRU CAN RGYINGS | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+0F8B | `ྋ` | TIBETAN SIGN GRU MED RGYINGS | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+0F8C | `ྌ` | TIBETAN SIGN INVERTED MCHU CAN | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+0F8D | `◌ྍ` | TIBETAN SUBJOINED SIGN LCE TSA CAN | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+0F8E | `◌ྎ` | TIBETAN SUBJOINED SIGN MCHU CAN | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+0F8F | `◌ྏ` | TIBETAN SUBJOINED SIGN INVERTED MCHU CAN | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+0FAC | `◌ྫྷ` | TIBETAN SUBJOINED LETTER DZHA | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+0FB0 | `◌ྰ` | TIBETAN SUBJOINED LETTER -A **★** | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+0FB8 | `◌ྸ` | TIBETAN SUBJOINED LETTER A **★** | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+0FB9 | `◌ྐྵ` | TIBETAN SUBJOINED LETTER KSSA | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+0FBE | `྾` | TIBETAN KU RU KHA | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+0FBF | `྿` | TIBETAN KU RU KHA BZHI MIG CAN | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+0FC0 | `࿀` | TIBETAN CANTILLATION SIGN HEAVY BEAT | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+0FC1 | `࿁` | TIBETAN CANTILLATION SIGN LIGHT BEAT | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+0FC2 | `࿂` | TIBETAN CANTILLATION SIGN CANG TE-U | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+0FC3 | `࿃` | TIBETAN CANTILLATION SIGN SBUB -CHAL | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+0FC4 | `࿄` | TIBETAN SYMBOL DRIL BU | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+0FC5 | `࿅` | TIBETAN SYMBOL RDO RJE | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+0FC6 | `◌࿆` | TIBETAN SYMBOL PADMA GDAN | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+0FC7 | `࿇` | TIBETAN SYMBOL RDO RJE RGYA GRAM | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+0FC8 | `࿈` | TIBETAN SYMBOL PHUR PA | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+0FC9 | `࿉` | TIBETAN SYMBOL NOR BU | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+0FCA | `࿊` | TIBETAN SYMBOL NOR BU NYIS -KHYIL | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+0FCB | `࿋` | TIBETAN SYMBOL NOR BU GSUM -KHYIL | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+0FCC | `࿌` | TIBETAN SYMBOL NOR BU BZHI -KHYIL | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+0FCE | `࿎` | TIBETAN SIGN RDEL NAG RDEL DKAR | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+0FCF | `࿏` | TIBETAN SIGN RDEL NAG GSUM | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+0FD0 | `࿐` | TIBETAN MARK BSKA- SHOG GI MGO RGYAN | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+0FD1 | `࿑` | TIBETAN MARK MNYAM YIG GI MGO RGYAN | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+0FD2 | `࿒` | TIBETAN MARK NYIS TSHEG | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+0FD3 | `࿓` | TIBETAN MARK INITIAL BRDA RNYING YIG MGO MDUN MA | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+0FD4 | `࿔` | TIBETAN MARK CLOSING BRDA RNYING YIG MGO SGAB MA | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+0FD5 | `࿕` | RIGHT-FACING SVASTI SIGN | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+0FD6 | `࿖` | LEFT-FACING SVASTI SIGN | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+0FD7 | `࿗` | RIGHT-FACING SVASTI SIGN WITH DOTS | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+0FD8 | `࿘` | LEFT-FACING SVASTI SIGN WITH DOTS | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+0FD9 | `࿙` | TIBETAN MARK LEADING MCHAN RTAGS | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+0FDA | `࿚` | TIBETAN MARK TRAILING MCHAN RTAGS | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |

## Urdu (329 candidates)

Blocks: U+0600–U+06FF, U+0750–U+077F, U+08A0–U+08FF.

| Code point | Character | Unicode name | Closest candidate | Possible Inditrans solution |
|---|---|---|---|---|
| U+0606 | `؆` | ARABIC-INDIC CUBE ROOT | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+0607 | `؇` | ARABIC-INDIC FOURTH ROOT | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+0608 | `؈` | ARABIC RAY | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+0609 | `؉` | ARABIC-INDIC PER MILLE SIGN | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+060A | `؊` | ARABIC-INDIC PER TEN THOUSAND SIGN | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+060B | `؋` | AFGHANI SIGN | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+060D | `؍` | ARABIC DATE SEPARATOR **★** | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+060E | `؎` | ARABIC POETIC VERSE SIGN | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+060F | `؏` | ARABIC SIGN MISRA | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+0610 | `◌ؐ` | ARABIC SIGN SALLALLAHOU ALAYHE WASSALLAM | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+0611 | `◌ؑ` | ARABIC SIGN ALAYHE ASSALLAM | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+0612 | `◌ؒ` | ARABIC SIGN RAHMATULLAH ALAYHE | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+0613 | `◌ؓ` | ARABIC SIGN RADI ALLAHOU ANHU | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+0614 | `◌ؔ` | ARABIC SIGN TAKHALLUS | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+0615 | `◌ؕ` | ARABIC SMALL HIGH TAH | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+0616 | `◌ؖ` | ARABIC SMALL HIGH LIGATURE ALEF WITH LAM WITH YEH | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+0617 | `◌ؗ` | ARABIC SMALL HIGH ZAIN | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+0618 | `◌ؘ` | ARABIC SMALL FATHA | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+0619 | `◌ؙ` | ARABIC SMALL DAMMA | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+061A | `◌ؚ` | ARABIC SMALL KASRA | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+061B | `؛` | ARABIC SEMICOLON **★** | `;` | Add a semicolon symbol token and target fallbacks. |
| U+061D | `؝` | ARABIC END OF TEXT MARK | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+061E | `؞` | ARABIC TRIPLE DOT PUNCTUATION MARK | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+061F | `؟` | ARABIC QUESTION MARK **★** | `?` | Add a question-mark symbol token and target fallbacks. |
| U+0620 | `ؠ` | ARABIC LETTER KASHMIRI YEH | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+0621 | `ء` | ARABIC LETTER HAMZA **★** | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+0623 | `أ` | ARABIC LETTER ALEF WITH HAMZA ABOVE | `ا` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+0624 | `ؤ` | ARABIC LETTER WAW WITH HAMZA ABOVE | `و` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+0625 | `إ` | ARABIC LETTER ALEF WITH HAMZA BELOW | `ا` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+0626 | `ئ` | ARABIC LETTER YEH WITH HAMZA ABOVE | `ي` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+0629 | `ة` | ARABIC LETTER TEH MARBUTA | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+063B | `ػ` | ARABIC LETTER KEHEH WITH TWO DOTS ABOVE | `ک` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+063C | `ؼ` | ARABIC LETTER KEHEH WITH THREE DOTS BELOW | `ک` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+063D | `ؽ` | ARABIC LETTER FARSI YEH WITH INVERTED V | `ی` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+063E | `ؾ` | ARABIC LETTER FARSI YEH WITH TWO DOTS ABOVE | `ی` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+063F | `ؿ` | ARABIC LETTER FARSI YEH WITH THREE DOTS ABOVE | `ی` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+0640 | `ـ` | ARABIC TATWEEL | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+064B | `◌ً` | ARABIC FATHATAN | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+064C | `◌ٌ` | ARABIC DAMMATAN | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+064D | `◌ٍ` | ARABIC KASRATAN | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+0653 | `◌ٓ` | ARABIC MADDAH ABOVE | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+0654 | `◌ٔ` | ARABIC HAMZA ABOVE | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+0655 | `◌ٕ` | ARABIC HAMZA BELOW | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+0656 | `◌ٖ` | ARABIC SUBSCRIPT ALEF | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+0657 | `◌ٗ` | ARABIC INVERTED DAMMA | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+0658 | `◌٘` | ARABIC MARK NOON GHUNNA | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+0659 | `◌ٙ` | ARABIC ZWARAKAY | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+065A | `◌ٚ` | ARABIC VOWEL SIGN SMALL V ABOVE | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+065B | `◌ٛ` | ARABIC VOWEL SIGN INVERTED SMALL V ABOVE | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+065C | `◌ٜ` | ARABIC VOWEL SIGN DOT BELOW | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+065D | `◌ٝ` | ARABIC REVERSED DAMMA | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+065E | `◌ٞ` | ARABIC FATHA WITH TWO DOTS | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+065F | `◌ٟ` | ARABIC WAVY HAMZA BELOW | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+0660 | `٠` | ARABIC-INDIC DIGIT ZERO | Numeric value `0` | Add numeric conversion; check neighboring numerals and target representation. |
| U+0661 | `١` | ARABIC-INDIC DIGIT ONE | Numeric value `1` | Add numeric conversion; check neighboring numerals and target representation. |
| U+0662 | `٢` | ARABIC-INDIC DIGIT TWO | Numeric value `2` | Add numeric conversion; check neighboring numerals and target representation. |
| U+0663 | `٣` | ARABIC-INDIC DIGIT THREE | Numeric value `3` | Add numeric conversion; check neighboring numerals and target representation. |
| U+0664 | `٤` | ARABIC-INDIC DIGIT FOUR | Numeric value `4` | Add numeric conversion; check neighboring numerals and target representation. |
| U+0665 | `٥` | ARABIC-INDIC DIGIT FIVE | Numeric value `5` | Add numeric conversion; check neighboring numerals and target representation. |
| U+0666 | `٦` | ARABIC-INDIC DIGIT SIX | Numeric value `6` | Add numeric conversion; check neighboring numerals and target representation. |
| U+0667 | `٧` | ARABIC-INDIC DIGIT SEVEN | Numeric value `7` | Add numeric conversion; check neighboring numerals and target representation. |
| U+0668 | `٨` | ARABIC-INDIC DIGIT EIGHT | Numeric value `8` | Add numeric conversion; check neighboring numerals and target representation. |
| U+0669 | `٩` | ARABIC-INDIC DIGIT NINE | Numeric value `9` | Add numeric conversion; check neighboring numerals and target representation. |
| U+066A | `٪` | ARABIC PERCENT SIGN | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+066B | `٫` | ARABIC DECIMAL SEPARATOR **★** | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+066C | `٬` | ARABIC THOUSANDS SEPARATOR **★** | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+066D | `٭` | ARABIC FIVE POINTED STAR | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+066E | `ٮ` | ARABIC LETTER DOTLESS BEH | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+066F | `ٯ` | ARABIC LETTER DOTLESS QAF | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+0670 | `◌ٰ` | ARABIC LETTER SUPERSCRIPT ALEF | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+0671 | `ٱ` | ARABIC LETTER ALEF WASLA | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+0672 | `ٲ` | ARABIC LETTER ALEF WITH WAVY HAMZA ABOVE | `ا` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+0673 | `ٳ` | ARABIC LETTER ALEF WITH WAVY HAMZA BELOW | `ا` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+0674 | `ٴ` | ARABIC LETTER HIGH HAMZA | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+0675 | `ٵ` | ARABIC LETTER HIGH HAMZA ALEF | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+0676 | `ٶ` | ARABIC LETTER HIGH HAMZA WAW | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+0677 | `ٷ` | ARABIC LETTER U WITH HAMZA ABOVE | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+0678 | `ٸ` | ARABIC LETTER HIGH HAMZA YEH | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+067A | `ٺ` | ARABIC LETTER TTEHEH | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+067B | `ٻ` | ARABIC LETTER BEEH | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+067C | `ټ` | ARABIC LETTER TEH WITH RING | `ت` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+067D | `ٽ` | ARABIC LETTER TEH WITH THREE DOTS ABOVE DOWNWARDS | `ت` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+067F | `ٿ` | ARABIC LETTER TEHEH | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+0680 | `ڀ` | ARABIC LETTER BEHEH | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+0681 | `ځ` | ARABIC LETTER HAH WITH HAMZA ABOVE | `ح` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+0682 | `ڂ` | ARABIC LETTER HAH WITH TWO DOTS VERTICAL ABOVE | `ح` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+0684 | `ڄ` | ARABIC LETTER DYEH | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+0685 | `څ` | ARABIC LETTER HAH WITH THREE DOTS ABOVE | `ح` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+0687 | `ڇ` | ARABIC LETTER TCHEHEH | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+0689 | `ډ` | ARABIC LETTER DAL WITH RING | `د` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+068A | `ڊ` | ARABIC LETTER DAL WITH DOT BELOW | `د` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+068B | `ڋ` | ARABIC LETTER DAL WITH DOT BELOW AND SMALL TAH | `د` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+068C | `ڌ` | ARABIC LETTER DAHAL | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+068D | `ڍ` | ARABIC LETTER DDAHAL | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+068E | `ڎ` | ARABIC LETTER DUL | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+068F | `ڏ` | ARABIC LETTER DAL WITH THREE DOTS ABOVE DOWNWARDS | `د` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+0690 | `ڐ` | ARABIC LETTER DAL WITH FOUR DOTS ABOVE | `د` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+0692 | `ڒ` | ARABIC LETTER REH WITH SMALL V | `ر` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+0693 | `ړ` | ARABIC LETTER REH WITH RING | `ر` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+0694 | `ڔ` | ARABIC LETTER REH WITH DOT BELOW | `ر` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+0695 | `ڕ` | ARABIC LETTER REH WITH SMALL V BELOW | `ر` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+0696 | `ږ` | ARABIC LETTER REH WITH DOT BELOW AND DOT ABOVE | `ر` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+0697 | `ڗ` | ARABIC LETTER REH WITH TWO DOTS ABOVE | `ر` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+0699 | `ڙ` | ARABIC LETTER REH WITH FOUR DOTS ABOVE | `ر` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+069A | `ښ` | ARABIC LETTER SEEN WITH DOT BELOW AND DOT ABOVE | `س` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+069B | `ڛ` | ARABIC LETTER SEEN WITH THREE DOTS BELOW | `س` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+069C | `ڜ` | ARABIC LETTER SEEN WITH THREE DOTS BELOW AND THREE DOTS ABOVE | `س` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+069D | `ڝ` | ARABIC LETTER SAD WITH TWO DOTS BELOW | `ص` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+069E | `ڞ` | ARABIC LETTER SAD WITH THREE DOTS ABOVE | `ص` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+069F | `ڟ` | ARABIC LETTER TAH WITH THREE DOTS ABOVE | `ط` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+06A0 | `ڠ` | ARABIC LETTER AIN WITH THREE DOTS ABOVE | `ع` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+06A1 | `ڡ` | ARABIC LETTER DOTLESS FEH | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+06A2 | `ڢ` | ARABIC LETTER FEH WITH DOT MOVED BELOW | `ف` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+06A3 | `ڣ` | ARABIC LETTER FEH WITH DOT BELOW | `ف` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+06A4 | `ڤ` | ARABIC LETTER VEH | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+06A5 | `ڥ` | ARABIC LETTER FEH WITH THREE DOTS BELOW | `ف` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+06A6 | `ڦ` | ARABIC LETTER PEHEH | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+06A7 | `ڧ` | ARABIC LETTER QAF WITH DOT ABOVE | `ق` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+06A8 | `ڨ` | ARABIC LETTER QAF WITH THREE DOTS ABOVE | `ق` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+06AA | `ڪ` | ARABIC LETTER SWASH KAF | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+06AB | `ګ` | ARABIC LETTER KAF WITH RING | `ك` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+06AC | `ڬ` | ARABIC LETTER KAF WITH DOT ABOVE | `ك` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+06AD | `ڭ` | ARABIC LETTER NG | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+06AE | `ڮ` | ARABIC LETTER KAF WITH THREE DOTS BELOW | `ك` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+06B0 | `ڰ` | ARABIC LETTER GAF WITH RING | `گ` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+06B1 | `ڱ` | ARABIC LETTER NGOEH | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+06B2 | `ڲ` | ARABIC LETTER GAF WITH TWO DOTS BELOW | `گ` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+06B3 | `ڳ` | ARABIC LETTER GUEH | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+06B4 | `ڴ` | ARABIC LETTER GAF WITH THREE DOTS ABOVE | `گ` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+06B5 | `ڵ` | ARABIC LETTER LAM WITH SMALL V | `ل` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+06B6 | `ڶ` | ARABIC LETTER LAM WITH DOT ABOVE | `ل` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+06B7 | `ڷ` | ARABIC LETTER LAM WITH THREE DOTS ABOVE | `ل` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+06B8 | `ڸ` | ARABIC LETTER LAM WITH THREE DOTS BELOW | `ل` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+06B9 | `ڹ` | ARABIC LETTER NOON WITH DOT BELOW | `ن` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+06BB | `ڻ` | ARABIC LETTER RNOON | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+06BC | `ڼ` | ARABIC LETTER NOON WITH RING | `ن` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+06BD | `ڽ` | ARABIC LETTER NOON WITH THREE DOTS ABOVE | `ن` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+06BF | `ڿ` | ARABIC LETTER TCHEH WITH DOT ABOVE | `چ` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+06C0 | `ۀ` | ARABIC LETTER HEH WITH YEH ABOVE | `ه` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+06C2 | `ۂ` | ARABIC LETTER HEH GOAL WITH HAMZA ABOVE | `ہ` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+06C3 | `ۃ` | ARABIC LETTER TEH MARBUTA GOAL | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+06C4 | `ۄ` | ARABIC LETTER WAW WITH RING | `و` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+06C5 | `ۅ` | ARABIC LETTER KIRGHIZ OE | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+06C6 | `ۆ` | ARABIC LETTER OE | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+06C7 | `ۇ` | ARABIC LETTER U | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+06C8 | `ۈ` | ARABIC LETTER YU | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+06C9 | `ۉ` | ARABIC LETTER KIRGHIZ YU | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+06CA | `ۊ` | ARABIC LETTER WAW WITH TWO DOTS ABOVE | `و` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+06CB | `ۋ` | ARABIC LETTER VE | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+06CD | `ۍ` | ARABIC LETTER YEH WITH TAIL | `ي` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+06CE | `ێ` | ARABIC LETTER YEH WITH SMALL V | `ي` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+06CF | `ۏ` | ARABIC LETTER WAW WITH DOT ABOVE | `و` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+06D0 | `ې` | ARABIC LETTER E | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+06D1 | `ۑ` | ARABIC LETTER YEH WITH THREE DOTS BELOW | `ي` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+06D3 | `ۓ` | ARABIC LETTER YEH BARREE WITH HAMZA ABOVE | `ے` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+06D5 | `ە` | ARABIC LETTER AE | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+06D6 | `◌ۖ` | ARABIC SMALL HIGH LIGATURE SAD WITH LAM WITH ALEF MAKSURA | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+06D7 | `◌ۗ` | ARABIC SMALL HIGH LIGATURE QAF WITH LAM WITH ALEF MAKSURA | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+06D8 | `◌ۘ` | ARABIC SMALL HIGH MEEM INITIAL FORM | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+06D9 | `◌ۙ` | ARABIC SMALL HIGH LAM ALEF | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+06DA | `◌ۚ` | ARABIC SMALL HIGH JEEM | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+06DB | `◌ۛ` | ARABIC SMALL HIGH THREE DOTS | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+06DC | `◌ۜ` | ARABIC SMALL HIGH SEEN | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+06DE | `۞` | ARABIC START OF RUB EL HIZB | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+06DF | `◌۟` | ARABIC SMALL HIGH ROUNDED ZERO | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+06E0 | `◌۠` | ARABIC SMALL HIGH UPRIGHT RECTANGULAR ZERO | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+06E1 | `◌ۡ` | ARABIC SMALL HIGH DOTLESS HEAD OF KHAH | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+06E2 | `◌ۢ` | ARABIC SMALL HIGH MEEM ISOLATED FORM | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+06E3 | `◌ۣ` | ARABIC SMALL LOW SEEN | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+06E4 | `◌ۤ` | ARABIC SMALL HIGH MADDA | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+06E5 | `ۥ` | ARABIC SMALL WAW | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+06E6 | `ۦ` | ARABIC SMALL YEH | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+06E7 | `◌ۧ` | ARABIC SMALL HIGH YEH | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+06E8 | `◌ۨ` | ARABIC SMALL HIGH NOON | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+06E9 | `۩` | ARABIC PLACE OF SAJDAH | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+06EA | `◌۪` | ARABIC EMPTY CENTRE LOW STOP | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+06EB | `◌۫` | ARABIC EMPTY CENTRE HIGH STOP | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+06EC | `◌۬` | ARABIC ROUNDED HIGH STOP WITH FILLED CENTRE | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+06ED | `◌ۭ` | ARABIC SMALL LOW MEEM | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+06EE | `ۮ` | ARABIC LETTER DAL WITH INVERTED V | `د` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+06EF | `ۯ` | ARABIC LETTER REH WITH INVERTED V | `ر` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+06FA | `ۺ` | ARABIC LETTER SHEEN WITH DOT BELOW | `ش` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+06FB | `ۻ` | ARABIC LETTER DAD WITH DOT BELOW | `ض` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+06FC | `ۼ` | ARABIC LETTER GHAIN WITH DOT BELOW | `غ` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+06FD | `۽` | ARABIC SIGN SINDHI AMPERSAND | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+06FE | `۾` | ARABIC SIGN SINDHI POSTPOSITION MEN | Not determined | Review symbol meaning; add a target token, expansion, or contextual handling. |
| U+06FF | `ۿ` | ARABIC LETTER HEH WITH INVERTED V | `ه` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+0750 | `ݐ` | ARABIC LETTER BEH WITH THREE DOTS HORIZONTALLY BELOW | `ب` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+0751 | `ݑ` | ARABIC LETTER BEH WITH DOT BELOW AND THREE DOTS ABOVE | `ب` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+0752 | `ݒ` | ARABIC LETTER BEH WITH THREE DOTS POINTING UPWARDS BELOW | `ب` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+0753 | `ݓ` | ARABIC LETTER BEH WITH THREE DOTS POINTING UPWARDS BELOW AND TWO DOTS ABOVE | `ب` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+0754 | `ݔ` | ARABIC LETTER BEH WITH TWO DOTS BELOW AND DOT ABOVE | `ب` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+0755 | `ݕ` | ARABIC LETTER BEH WITH INVERTED SMALL V BELOW | `ب` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+0756 | `ݖ` | ARABIC LETTER BEH WITH SMALL V | `ب` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+0757 | `ݗ` | ARABIC LETTER HAH WITH TWO DOTS ABOVE | `ح` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+0758 | `ݘ` | ARABIC LETTER HAH WITH THREE DOTS POINTING UPWARDS BELOW | `ح` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+0759 | `ݙ` | ARABIC LETTER DAL WITH TWO DOTS VERTICALLY BELOW AND SMALL TAH | `د` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+075A | `ݚ` | ARABIC LETTER DAL WITH INVERTED SMALL V BELOW | `د` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+075B | `ݛ` | ARABIC LETTER REH WITH STROKE | `ر` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+075C | `ݜ` | ARABIC LETTER SEEN WITH FOUR DOTS ABOVE | `س` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+075D | `ݝ` | ARABIC LETTER AIN WITH TWO DOTS ABOVE | `ع` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+075E | `ݞ` | ARABIC LETTER AIN WITH THREE DOTS POINTING DOWNWARDS ABOVE | `ع` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+075F | `ݟ` | ARABIC LETTER AIN WITH TWO DOTS VERTICALLY ABOVE | `ع` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+0760 | `ݠ` | ARABIC LETTER FEH WITH TWO DOTS BELOW | `ف` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+0761 | `ݡ` | ARABIC LETTER FEH WITH THREE DOTS POINTING UPWARDS BELOW | `ف` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+0762 | `ݢ` | ARABIC LETTER KEHEH WITH DOT ABOVE | `ک` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+0763 | `ݣ` | ARABIC LETTER KEHEH WITH THREE DOTS ABOVE | `ک` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+0764 | `ݤ` | ARABIC LETTER KEHEH WITH THREE DOTS POINTING UPWARDS BELOW | `ک` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+0765 | `ݥ` | ARABIC LETTER MEEM WITH DOT ABOVE | `م` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+0766 | `ݦ` | ARABIC LETTER MEEM WITH DOT BELOW | `م` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+0767 | `ݧ` | ARABIC LETTER NOON WITH TWO DOTS BELOW | `ن` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+0769 | `ݩ` | ARABIC LETTER NOON WITH SMALL V | `ن` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+076A | `ݪ` | ARABIC LETTER LAM WITH BAR | `ل` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+076B | `ݫ` | ARABIC LETTER REH WITH TWO DOTS VERTICALLY ABOVE | `ر` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+076C | `ݬ` | ARABIC LETTER REH WITH HAMZA ABOVE | `ر` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+076D | `ݭ` | ARABIC LETTER SEEN WITH TWO DOTS VERTICALLY ABOVE | `س` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+076E | `ݮ` | ARABIC LETTER HAH WITH SMALL ARABIC LETTER TAH BELOW | `ح` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+076F | `ݯ` | ARABIC LETTER HAH WITH SMALL ARABIC LETTER TAH AND TWO DOTS | `ح` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+0770 | `ݰ` | ARABIC LETTER SEEN WITH SMALL ARABIC LETTER TAH AND TWO DOTS | `س` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+0771 | `ݱ` | ARABIC LETTER REH WITH SMALL ARABIC LETTER TAH AND TWO DOTS | `ر` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+0772 | `ݲ` | ARABIC LETTER HAH WITH SMALL ARABIC LETTER TAH ABOVE | `ح` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+0773 | `ݳ` | ARABIC LETTER ALEF WITH EXTENDED ARABIC-INDIC DIGIT TWO ABOVE | `ا` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+0774 | `ݴ` | ARABIC LETTER ALEF WITH EXTENDED ARABIC-INDIC DIGIT THREE ABOVE | `ا` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+0775 | `ݵ` | ARABIC LETTER FARSI YEH WITH EXTENDED ARABIC-INDIC DIGIT TWO ABOVE | `ی` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+0776 | `ݶ` | ARABIC LETTER FARSI YEH WITH EXTENDED ARABIC-INDIC DIGIT THREE ABOVE | `ی` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+0777 | `ݷ` | ARABIC LETTER FARSI YEH WITH EXTENDED ARABIC-INDIC DIGIT FOUR BELOW | `ی` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+0778 | `ݸ` | ARABIC LETTER WAW WITH EXTENDED ARABIC-INDIC DIGIT TWO ABOVE | `و` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+0779 | `ݹ` | ARABIC LETTER WAW WITH EXTENDED ARABIC-INDIC DIGIT THREE ABOVE | `و` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+077A | `ݺ` | ARABIC LETTER YEH BARREE WITH EXTENDED ARABIC-INDIC DIGIT TWO ABOVE | `ے` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+077B | `ݻ` | ARABIC LETTER YEH BARREE WITH EXTENDED ARABIC-INDIC DIGIT THREE ABOVE | `ے` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+077C | `ݼ` | ARABIC LETTER HAH WITH EXTENDED ARABIC-INDIC DIGIT FOUR BELOW | `ح` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+077D | `ݽ` | ARABIC LETTER SEEN WITH EXTENDED ARABIC-INDIC DIGIT FOUR ABOVE | `س` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+077E | `ݾ` | ARABIC LETTER SEEN WITH INVERTED V | `س` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+077F | `ݿ` | ARABIC LETTER KAF WITH TWO DOTS ABOVE | `ك` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+08A0 | `ࢠ` | ARABIC LETTER BEH WITH SMALL V BELOW | `ب` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+08A1 | `ࢡ` | ARABIC LETTER BEH WITH HAMZA ABOVE | `ب` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+08A2 | `ࢢ` | ARABIC LETTER JEEM WITH TWO DOTS ABOVE | `ج` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+08A3 | `ࢣ` | ARABIC LETTER TAH WITH TWO DOTS ABOVE | `ط` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+08A4 | `ࢤ` | ARABIC LETTER FEH WITH DOT BELOW AND THREE DOTS ABOVE | `ف` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+08A5 | `ࢥ` | ARABIC LETTER QAF WITH DOT BELOW | `ق` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+08A6 | `ࢦ` | ARABIC LETTER LAM WITH DOUBLE BAR | `ل` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+08A7 | `ࢧ` | ARABIC LETTER MEEM WITH THREE DOTS ABOVE | `م` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+08A8 | `ࢨ` | ARABIC LETTER YEH WITH TWO DOTS BELOW AND HAMZA ABOVE | `ي` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+08A9 | `ࢩ` | ARABIC LETTER YEH WITH TWO DOTS BELOW AND DOT ABOVE | `ي` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+08AA | `ࢪ` | ARABIC LETTER REH WITH LOOP | `ر` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+08AB | `ࢫ` | ARABIC LETTER WAW WITH DOT WITHIN | `و` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+08AC | `ࢬ` | ARABIC LETTER ROHINGYA YEH | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+08AD | `ࢭ` | ARABIC LETTER LOW ALEF | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+08AE | `ࢮ` | ARABIC LETTER DAL WITH THREE DOTS BELOW | `د` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+08AF | `ࢯ` | ARABIC LETTER SAD WITH THREE DOTS BELOW | `ص` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+08B0 | `ࢰ` | ARABIC LETTER GAF WITH INVERTED STROKE | `گ` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+08B1 | `ࢱ` | ARABIC LETTER STRAIGHT WAW | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+08B2 | `ࢲ` | ARABIC LETTER ZAIN WITH INVERTED V ABOVE | `ز` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+08B3 | `ࢳ` | ARABIC LETTER AIN WITH THREE DOTS BELOW | `ع` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+08B4 | `ࢴ` | ARABIC LETTER KAF WITH DOT BELOW | `ك` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+08B5 | `ࢵ` | ARABIC LETTER QAF WITH DOT BELOW AND NO DOTS ABOVE | `ق` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+08B6 | `ࢶ` | ARABIC LETTER BEH WITH SMALL MEEM ABOVE | `ب` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+08B7 | `ࢷ` | ARABIC LETTER PEH WITH SMALL MEEM ABOVE | `پ` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+08B8 | `ࢸ` | ARABIC LETTER TEH WITH SMALL TEH ABOVE | `ت` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+08B9 | `ࢹ` | ARABIC LETTER REH WITH SMALL NOON ABOVE | `ر` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+08BA | `ࢺ` | ARABIC LETTER YEH WITH TWO DOTS BELOW AND SMALL NOON ABOVE | `ي` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+08BB | `ࢻ` | ARABIC LETTER AFRICAN FEH | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+08BC | `ࢼ` | ARABIC LETTER AFRICAN QAF | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+08BD | `ࢽ` | ARABIC LETTER AFRICAN NOON | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+08BE | `ࢾ` | ARABIC LETTER PEH WITH SMALL V | `پ` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+08BF | `ࢿ` | ARABIC LETTER TEH WITH SMALL V | `ت` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+08C0 | `ࣀ` | ARABIC LETTER TTEH WITH SMALL V | `ٹ` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+08C1 | `ࣁ` | ARABIC LETTER TCHEH WITH SMALL V | `چ` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+08C2 | `ࣂ` | ARABIC LETTER KEHEH WITH SMALL V | `ک` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+08C3 | `ࣃ` | ARABIC LETTER GHAIN WITH THREE DOTS ABOVE | `غ` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+08C4 | `ࣄ` | ARABIC LETTER AFRICAN QAF WITH THREE DOTS ABOVE | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+08C5 | `ࣅ` | ARABIC LETTER JEEM WITH THREE DOTS ABOVE | `ج` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+08C6 | `ࣆ` | ARABIC LETTER JEEM WITH THREE DOTS BELOW | `ج` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+08C7 | `ࣇ` | ARABIC LETTER LAM WITH SMALL ARABIC LETTER TAH ABOVE | `ل` (name-based candidate; verify) | Verify phonetic value and language; add a reader alias if appropriate. |
| U+08C8 | `ࣈ` | ARABIC LETTER GRAF | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+08C9 | `ࣉ` | ARABIC SMALL FARSI YEH | Not determined | Review language and sound; add an alias or token sequence if a close match exists. |
| U+08CA | `◌࣊` | ARABIC SMALL HIGH FARSI YEH | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+08CB | `◌࣋` | ARABIC SMALL HIGH YEH BARREE WITH TWO DOTS BELOW | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+08CC | `◌࣌` | ARABIC SMALL HIGH WORD SAH | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+08CD | `◌࣍` | ARABIC SMALL HIGH ZAH | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+08CE | `◌࣎` | ARABIC LARGE ROUND DOT ABOVE | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+08CF | `◌࣏` | ARABIC LARGE ROUND DOT BELOW | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+08D0 | `◌࣐` | ARABIC SUKUN BELOW | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+08D1 | `◌࣑` | ARABIC LARGE CIRCLE BELOW | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+08D2 | `◌࣒` | ARABIC LARGE ROUND DOT INSIDE CIRCLE BELOW | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+08D3 | `◌࣓` | ARABIC SMALL LOW WAW | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+08D4 | `◌ࣔ` | ARABIC SMALL HIGH WORD AR-RUB | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+08D5 | `◌ࣕ` | ARABIC SMALL HIGH SAD | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+08D6 | `◌ࣖ` | ARABIC SMALL HIGH AIN | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+08D7 | `◌ࣗ` | ARABIC SMALL HIGH QAF | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+08D8 | `◌ࣘ` | ARABIC SMALL HIGH NOON WITH KASRA | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+08D9 | `◌ࣙ` | ARABIC SMALL LOW NOON WITH KASRA | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+08DA | `◌ࣚ` | ARABIC SMALL HIGH WORD ATH-THALATHA | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+08DB | `◌ࣛ` | ARABIC SMALL HIGH WORD AS-SAJDA | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+08DC | `◌ࣜ` | ARABIC SMALL HIGH WORD AN-NISF | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+08DD | `◌ࣝ` | ARABIC SMALL HIGH WORD SAKTA | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+08DE | `◌ࣞ` | ARABIC SMALL HIGH WORD QIF | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+08DF | `◌ࣟ` | ARABIC SMALL HIGH WORD WAQFA | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+08E0 | `◌࣠` | ARABIC SMALL HIGH FOOTNOTE MARKER | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+08E1 | `◌࣡` | ARABIC SMALL HIGH SIGN SAFHA | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+08E3 | `◌ࣣ` | ARABIC TURNED DAMMA BELOW | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+08E4 | `◌ࣤ` | ARABIC CURLY FATHA | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+08E5 | `◌ࣥ` | ARABIC CURLY DAMMA | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+08E6 | `◌ࣦ` | ARABIC CURLY KASRA | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+08E7 | `◌ࣧ` | ARABIC CURLY FATHATAN | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+08E8 | `◌ࣨ` | ARABIC CURLY DAMMATAN | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+08E9 | `◌ࣩ` | ARABIC CURLY KASRATAN | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+08EA | `◌࣪` | ARABIC TONE ONE DOT ABOVE | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+08EB | `◌࣫` | ARABIC TONE TWO DOTS ABOVE | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+08EC | `◌࣬` | ARABIC TONE LOOP ABOVE | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+08ED | `◌࣭` | ARABIC TONE ONE DOT BELOW | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+08EE | `◌࣮` | ARABIC TONE TWO DOTS BELOW | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+08EF | `◌࣯` | ARABIC TONE LOOP BELOW | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+08F0 | `◌ࣰ` | ARABIC OPEN FATHATAN | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+08F1 | `◌ࣱ` | ARABIC OPEN DAMMATAN | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+08F2 | `◌ࣲ` | ARABIC OPEN KASRATAN | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+08F3 | `◌ࣳ` | ARABIC SMALL HIGH WAW | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+08F4 | `◌ࣴ` | ARABIC FATHA WITH RING | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+08F5 | `◌ࣵ` | ARABIC FATHA WITH DOT ABOVE | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+08F6 | `◌ࣶ` | ARABIC KASRA WITH DOT BELOW | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+08F7 | `◌ࣷ` | ARABIC LEFT ARROWHEAD ABOVE | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+08F8 | `◌ࣸ` | ARABIC RIGHT ARROWHEAD ABOVE | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+08F9 | `◌ࣹ` | ARABIC LEFT ARROWHEAD BELOW | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+08FA | `◌ࣺ` | ARABIC RIGHT ARROWHEAD BELOW | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+08FB | `◌ࣻ` | ARABIC DOUBLE RIGHT ARROWHEAD ABOVE | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+08FC | `◌ࣼ` | ARABIC DOUBLE RIGHT ARROWHEAD ABOVE WITH DOT | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+08FD | `◌ࣽ` | ARABIC RIGHT ARROWHEAD ABOVE WITH DOT | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+08FE | `◌ࣾ` | ARABIC DAMMA WITH DOT | Not determined | Review mark function; add a diacritic alias or contextual handling. |
| U+08FF | `◌ࣿ` | ARABIC MARK SIDEWAYS NOON GHUNNA | Not determined | Review mark function; add a diacritic alias or contextual handling. |
