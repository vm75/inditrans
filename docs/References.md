# References
## Keyboards
* https://www.lexilogos.com/keyboard/devanagari.htm
## Script info
* https://aksharamukha.appspot.com/script-matrix
* https://aksharamukha.appspot.com/roman
## ITRANS
* https://help.keyman.com/keyboard/itrans_devanagari_hindi/1.2.0/itrans_devanagari_hindi
* https://en.wikipedia.org/wiki/ITRANS
* https://sanskritdocuments.org/learning_tools/sanskritvedic.html
* https://docs.google.com/spreadsheets/d/14wZl8zCa4khZV3El2VGoqurKBLGx21mbS-yORi4w7Qo/edit#gid=0
## ISO 15919
* https://en.wikipedia.org/wiki/ISO_15919
## ISO/IEC 8859
* https://en.wikipedia.org/wiki/ISO/IEC_8859
## IPA
* http://www.sanskritweb.net/deutsch/ipa_sans.pdf
* https://en.wikipedia.org/wiki/Help:IPA/Sanskrit
## IAST
* https://en.wikipedia.org/wiki/International_Alphabet_of_Sanskrit_Transliteration
* https://en.wikipedia.org/wiki/Template:IAST

# Code reference for alternatives
* https://raw.githubusercontent.com/virtualvinodh/aksharamukha-python/main/aksharamukha/ConvertFix.py
* https://raw.githubusercontent.com/virtualvinodh/aksharamukha-python/main/aksharamukha/PostProcess.py
* https://raw.githubusercontent.com/virtualvinodh/aksharamukha-python/main/aksharamukha/PreProcess.py
* https://raw.githubusercontent.com/virtualvinodh/aksharamukha/master/aksharamukha-back/resources/script_mapping/script_mapping.json
* https://github.com/indic-transliteration/common_maps/blob/master/roman/iso.toml
* https://github.com/indic-transliteration/common_maps/blob/master/roman/iast.toml

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

## Gurmukhi adhak
* https://www.discoversikhism.com/punjabi/punjabi_gurmukhi_alphabet.html

## Alternate encodings
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
* { TokenType::Symbol,          ScriptType::Latin,  14 /* ꣳ */,       "\\m+" },
* { TokenType::Symbol,          ScriptType::Latin,  14 /* ꣳ */,       "{\\m+}" },
* { TokenType::Symbol,          ScriptType::Latin,  14 /* ꣳ */,       "\\м+" },
* { TokenType::Symbol,          ScriptType::Latin,  15 /* ꣴ */,       "\\m++" },
* { TokenType::Symbol,          ScriptType::Latin,  15 /* ꣴ */,       "\\м++" },
