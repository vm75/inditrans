# References
* https://aksharamukha.appspot.com/script-matrix
* https://aksharamukha.appspot.com/roman
## ITRANS
* https://help.keyman.com/keyboard/itrans_devanagari_hindi/1.2.0/itrans_devanagari_hindi
* https://en.wikipedia.org/wiki/ITRANS
* https://sanskritdocuments.org/learning_tools/sanskritvedic.html
## ISO
* https://en.wikipedia.org/wiki/ISO_15919
## IPA
* http://www.sanskritweb.net/deutsch/ipa_sans.pdf
* https://en.wikipedia.org/wiki/Help:IPA/Sanskrit
## IAST
* https://en.wikipedia.org/wiki/International_Alphabet_of_Sanskrit_Transliteration
* https://en.wikipedia.org/wiki/Template:IAST
## optitrans
* https://github.com/indic-transliteration/common_maps/blob/master/roman/optitrans.toml
    "shankara", sanscript.OPTITRANS == >"sha~Nkara", sanscript.ITRANS
    "manjIra", sanscript.OPTITRANS == >"ma~njIra", sanscript.ITRANS
    "praBA", sanscript.OPTITRANS ==> "prabhA", sanscript.ITRANS
    "pRRS", sanscript.OPTITRANS ==> "pRRISh", sanscript.ITRANS
    "pRcCa", sanscript.OPTITRANS ==> "pRRichCha", sanscript.ITRANS
    "R", sanscript.OPTITRANS ==> "RRi", sanscript.ITRANS
    "Rc", sanscript.OPTITRANS ==> "RRich", sanscript.ITRANS
    "sha~Nkara", sanscript.ITRANS ==> "shankara", sanscript.OPTITRANS
    "ma~njIra", sanscript.ITRANS ==> "manjIra", sanscript.OPTITRANS

# Code reference for alternatives
* https://raw.githubusercontent.com/virtualvinodh/aksharamukha-python/main/aksharamukha/ConvertFix.py
* https://raw.githubusercontent.com/virtualvinodh/aksharamukha-python/main/aksharamukha/PostProcess.py
* https://raw.githubusercontent.com/virtualvinodh/aksharamukha-python/main/aksharamukha/PreProcess.py
* https://raw.githubusercontent.com/virtualvinodh/aksharamukha/master/aksharamukha-back/resources/script_mapping/script_mapping.json
* https://github.com/indic-transliteration/common_maps/blob/master/roman/iso.toml
* https://github.com/indic-transliteration/common_maps/blob/master/roman/iast.toml

# Conjuncts in ITRANS
A full (live) consonant with visible virama in middle of words can be typed using ^, e.g. k ^ S produces क्‌ष.

## Zero Width Non Joiner
To type a Zero Width Non Joiner, _ can be typed between the letters, for example: a _ i will generate अ‌इ , k _ S will generate क्‌ष .

## Zero Width Joiner (ZWJ)
To type a Zero Width Joiner, + can be typed between the letters, for example: k + S will generate क्‍ष , while kS generates क्ष.

## References
* https://en.wikipedia.org/wiki/Zero-width_joiner
* https://en.wikipedia.org/wiki/Zero-width_non-joiner
* https://en.wikipedia.org/wiki/Zero-width_space

# Sample texts
* http://gretil.sub.uni-goettingen.de/gretil/1_sanskr/1_veda/1_sam/avs_acu.htm
* https://docs.google.com/spreadsheets/d/13iSPO2DIVko2O_WGQtpgJ8_Cd1v6lB2gcrfhzmkKlaI/edit#gid=0 (converter spreadsheet)

# Fonts
* http://www.virtualvinodh.com/projects/vajranandi
* https://www.fonts4free.net/samarkan-font.html

# Special handling
## Nutka
* https://en.wikipedia.org/wiki/Nuqta
## Malayalam chillu
* https://en.wiktionary.org/wiki/chillu
## Gurmukhi adhak
* https://www.discoversikhism.com/punjabi/punjabi_gurmukhi_alphabet.html

## Alternate encodings
* 'ŭ' => 'u\u00D7' # special explicit Virama
* { TokenType::Accent,          ScriptType::Indic,   1 /*꠰  ॑ */,       "◌॑" },
* { TokenType::Accent,          ScriptType::Latin,   0 /*꠰  ॒ */,       "̱" },
* { TokenType::Accent,          ScriptType::Latin,   0 /*꠰  ॒ */,       "↓" },
* { TokenType::Accent,          ScriptType::Latin,   0 /*꠰  ॒ */,       "\\_" },
* { TokenType::Accent,          ScriptType::Latin,   0 /*꠰  ॒ */,       "\\`" },
* { TokenType::Accent,          ScriptType::Latin,   0 /*꠰  ॒ */,       "'" },
* { TokenType::Accent,          ScriptType::Latin,   0 /*꠰  ॒ */,       "`" },
* { TokenType::Accent,          ScriptType::Latin,   1 /*꠰  ॑ */,       "̍" },
* { TokenType::Accent,          ScriptType::Latin,   1 /*꠰  ॑ */,       "↑" },
* { TokenType::Accent,          ScriptType::Latin,   1 /*꠰  ॑ */,       "\\'" },
* { TokenType::Accent,          ScriptType::Latin,   1 /*꠰  ॑ */,       "\\’" },
* { TokenType::Accent,          ScriptType::Latin,   1 /*꠰  ॑ */,       "̭" },
* { TokenType::Accent,          ScriptType::Latin,   2 /*꠰  ᳚ */,       "̎" },
* { TokenType::Accent,          ScriptType::Latin,   2 /*꠰  ᳚ */,       "↑↑" },
* { TokenType::Accent,          ScriptType::Latin,   2 /*꠰  ᳚ */,       "\\\"" },
* { TokenType::Accent,          ScriptType::Latin,   2 /*꠰  ᳚ */,       "\\''" },
* { TokenType::Accent,          ScriptType::Latin,   2 /*꠰  ᳚ */,       "\\’’" },
* { TokenType::CommonDiacritic, ScriptType::Latin,   0 /* ँ */,       "~" },
* { TokenType::CommonDiacritic, ScriptType::Latin,   0 /* ँ */,       "m̐" },
* { TokenType::CommonDiacritic, ScriptType::Latin,   0 /* ँ */,       "ṁ" },
* { TokenType::CommonDiacritic, ScriptType::Latin,   2 /* ः */,       "ḥ" },
* { TokenType::CommonDiacritic, ScriptType::Latin,   2 /* ः */,       "ḥ" },
* { TokenType::Symbol,          ScriptType::Latin,  10 /* ॐ */,      "o3m" },
* { TokenType::Symbol,          ScriptType::Latin,  10 /* ॐ */,      "OM" },
* { TokenType::Symbol,          ScriptType::Latin,  10 /* ॐ */,      "AUM" },
* { TokenType::Symbol,          ScriptType::Latin,  10 /* ॐ */,      "oṃ" },
* { TokenType::Symbol,          ScriptType::Latin,  10 /* ॐ */,      "ŏṃ" },
* { TokenType::Symbol,          ScriptType::Latin,  12 /* ꠰ */,        "|" },
* { TokenType::Symbol,          ScriptType::Latin,  12 /* ꠰ */,        "." },
* { TokenType::Symbol,          ScriptType::Latin,  13 /* ॥ */,       "||" },
* { TokenType::Symbol,          ScriptType::Latin,  13 /* ॥ */,       "꠰꠰" },
* { TokenType::Symbol,          ScriptType::Latin,  13 /* ॥ */,       ".." },
* { TokenType::Symbol,          ScriptType::Latin,  14 /* ꣳ */,       "gͫ" },
* { TokenType::Symbol,          ScriptType::Latin,  14 /* ꣳ */,       "\\m+" },
* { TokenType::Symbol,          ScriptType::Latin,  14 /* ꣳ */,       "{\\m+}" },
* { TokenType::Symbol,          ScriptType::Latin,  14 /* ꣳ */,       "\\м+" },
* { TokenType::Symbol,          ScriptType::Latin,  15 /* ꣴ */,       "gͫ̄" },
* { TokenType::Symbol,          ScriptType::Latin,  15 /* ꣴ */,       "\\m++" },
* { TokenType::Symbol,          ScriptType::Latin,  15 /* ꣴ */,       "\\м++" },
