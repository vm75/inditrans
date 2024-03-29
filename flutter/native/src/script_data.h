#pragma once

#define Z "\u0000"
#define E "\u0001"

#define VEDIC "v\u0000"
#define INDIC "i\u0000"
#define TAMIL "t\u0000"
#define LATIN "l\u0000"

#define VOWELS "v\u0000"
#define VOWELMARKS "m\u0000"
#define CONSONANTS "c\u0000"
#define OTHERDIACRITICS "o\u0000"
#define SYMBOLS "s\u0000"
#define VEDICSYMBOLS "S\u0000"
#define EQUIVALENTS "E\u0000"
#define ALIASES "A\u0000"
#define LANGUAGES "l\u0000"

// clang-format off

const char scriptData[] =
  "assamese" Z VEDIC
    VOWELS "অ" Z "আ" Z "ই" Z "ঈ" Z "উ" Z "ঊ" Z "ঋ" Z "ৠ" Z "ঌ" Z "ৡ" Z "এ᳴" Z "এ" Z "ঐ" Z "ও᳴" Z "ও" Z "ঔ" Z "এ᳴" Z "এˇ" Z "আ᳴" Z E
    VOWELMARKS "্" Z "া" Z "ি" Z "ী" Z "ু" Z "ূ" Z "ৃ" Z "ৄ" Z "ৢ" Z "ৣ" Z "ে᳴" Z "ে" Z "ৈ" Z "ো᳴" Z "ো" Z "ৌ" Z "ে᳴" Z "েˇ" Z "া᳴" Z E
    CONSONANTS "ক" Z "খ" Z "গ" Z "ঘ" Z "ঙ" Z "চ" Z "ছ" Z "জ" Z "ঝ" Z "ঞ" Z "ট" Z "ঠ" Z "ড" Z "ঢ" Z "ণ" Z "ত" Z "থ" Z "দ" Z "ধ" Z "ন" Z "প" Z "ফ" Z "ব" Z "ভ" Z "ম" Z "য" Z "ৰ" Z "ল" Z "ৱ" Z "শ" Z "ষ" Z "স" Z "হ" Z "ল়" Z "ষ়" Z "ৰ়" Z "ন়" Z "ক়" Z "খ়" Z "গ়" Z "জ়" Z "ড়" Z "ঢ়" Z "ফ়" Z "য়" Z "ঁˆগ" Z "ঁˆজ" Z "ঁˆড" Z "ঁˆদ" Z "ঁˆব" Z E
    OTHERDIACRITICS "ঁ" Z "ং" Z "ঃ" Z "ঃʼ" Z E
    SYMBOLS "০" Z "১" Z "২" Z "৩" Z "৪" Z "৫" Z "৬" Z "৭" Z "৮" Z "৯" Z "ওঁ" Z "।" Z "॥" Z E
    VEDICSYMBOLS "ঽ" Z "ꣳ" Z "ꣴ" Z E
    EQUIVALENTS
      "ৰ" Z "র" Z E
      "ৱ" Z "ভ়" Z E
      "ৰ়" Z "র়" Z E
    E
  E
  "bengali" Z VEDIC
    ALIASES "bangla" Z E
    VOWELS "অ" Z "আ" Z "ই" Z "ঈ" Z "উ" Z "ঊ" Z "ঋ" Z "ৠ" Z "ঌ" Z "ৡ" Z "এ᳴" Z "এ" Z "ঐ" Z "ও᳴" Z "ও" Z "ঔ" Z "এ᳴" Z "এˇ" Z "আ᳴" Z E
    VOWELMARKS "্" Z "া" Z "ি" Z "ী" Z "ু" Z "ূ" Z "ৃ" Z "ৄ" Z "ৢ" Z "ৣ" Z "ে᳴" Z "ে" Z "ৈ" Z "ো᳴" Z "ো" Z "ৌ" Z "ে᳴" Z "েˇ" Z "া᳴" Z E
    CONSONANTS "ক" Z "খ" Z "গ" Z "ঘ" Z "ঙ" Z "চ" Z "ছ" Z "জ" Z "ঝ" Z "ঞ" Z "ট" Z "ঠ" Z "ড" Z "ঢ" Z "ণ" Z "ত" Z "থ" Z "দ" Z "ধ" Z "ন" Z "প" Z "ফ" Z "ব" Z "ভ" Z "ম" Z "য" Z "র" Z "ল" Z "ভ়" Z "শ" Z "ষ" Z "স" Z "হ" Z "ল়" Z "ষ়" Z "র়" Z "ন়" Z "ক়" Z "খ়" Z "গ়" Z "জ়" Z "ড়" Z "ঢ়" Z "ফ়" Z "য়" Z "ঁˆগ" Z "ঁˆজ" Z "ঁˆড" Z "ঁˆদ" Z "ঁˆব" Z E
    OTHERDIACRITICS "ঁ" Z "ং" Z "ঃ" Z "ঃʼ" Z E
    SYMBOLS "০" Z "১" Z "২" Z "৩" Z "৪" Z "৫" Z "৬" Z "৭" Z "৮" Z "৯" Z "ওঁ" Z "।" Z "॥" Z E
    VEDICSYMBOLS "ঽ" Z "ꣳ" Z "ꣴ" Z E
    EQUIVALENTS
      "র" Z "ৰ" Z E
      "ভ়" Z "ৱ" Z E
      "র়" Z "ৰ়" Z E
    E
  E
  "burmese" Z INDIC
    VOWELS "အ" Z "အာ" Z "ဣ" Z "ဤ" Z "ဥ" Z "ဦ" Z "ၒ" Z "ၓ" Z "ၔ" Z "ၕ" Z "ဧʽ" Z "ဧ" Z "အဲ" Z "ဩʽ" Z "ဩ" Z "ဪ" Z "ဧʽ" Z "ဧʽ" Z "အာʽ" Z E
    VOWELMARKS "်" Z "ာ" Z "ိ" Z "ီ" Z "ု" Z "ူ" Z "ၖ" Z "ၗ" Z "ၘ" Z "ၙ" Z "ေʽ" Z "ေ" Z "ဲ" Z "ောʽ" Z "ော" Z "ော်" Z "ေʽ" Z "ေʽ" Z "ာʽ" Z E
    CONSONANTS "က" Z "ခ" Z "ဂ" Z "ဃ" Z "င" Z "စ" Z "ဆ" Z "ဇ" Z "ဈ" Z "ဉ" Z "ဋ" Z "ဌ" Z "ဍ" Z "ဎ" Z "ဏ" Z "တ" Z "ထ" Z "ဒ" Z "ဓ" Z "န" Z "ပ" Z "ဖ" Z "ဗ" Z "ဘ" Z "မ" Z "ယ" Z "ရ" Z "လ" Z "ဝ" Z "ၐ" Z "ၑ" Z "သ" Z "ဟ" Z "ဠ" Z "ဠʽ" Z "ရʽ" Z "နʽ" Z "ကʽ" Z "ခʽ" Z "ဂʽ" Z "ဇʽ" Z "ဍʽ" Z "ဎʽ" Z "ဖʽ" Z "ယʽ" Z "ံဂʽ" Z "ံဇʽ" Z "ံဍʽ" Z "ံဒʽ" Z "ံဗʽ" Z E
    OTHERDIACRITICS "ံʽ" Z "ံ" Z "း" Z "းʽ" Z E
    SYMBOLS "၀" Z "၁" Z "၂" Z "၃" Z "၄" Z "၅" Z "၆" Z "၇" Z "၈" Z "၉" Z "ဥုံ" Z "၊" Z "။" Z E
    VEDICSYMBOLS "'" Z "ꣳ" Z "ꣴ" Z E
    EQUIVALENTS
      "x:0" Z "{}" Z "^^" Z E
      "x:1" Z "()" Z "^" Z E
    E
  E
  "devanagari" Z VEDIC
    ALIASES "hindi" Z "sanskrit" Z E
    VOWELS "अ" Z "आ" Z "इ" Z "ई" Z "उ" Z "ऊ" Z "ऋ" Z "ॠ" Z "ऌ" Z "ॡ" Z "ऎ" Z "ए" Z "ऐ" Z "ऒ" Z "ओ" Z "औ" Z "ऍ" Z "एॕ" Z "ऑ" Z E
    VOWELMARKS "्" Z "ा" Z "ि" Z "ी" Z "ु" Z "ू" Z "ृ" Z "ॄ" Z "ॢ" Z "ॣ" Z "ॆ" Z "े" Z "ै" Z "ॊ" Z "ो" Z "ौ" Z "ॅ" Z "ॕ" Z "ॉ" Z E
    CONSONANTS "क" Z "ख" Z "ग" Z "घ" Z "ङ" Z "च" Z "छ" Z "ज" Z "झ" Z "ञ" Z "ट" Z "ठ" Z "ड" Z "ढ" Z "ण" Z "त" Z "थ" Z "द" Z "ध" Z "न" Z "प" Z "फ" Z "ब" Z "भ" Z "म" Z "य" Z "र" Z "ल" Z "व" Z "श" Z "ष" Z "स" Z "ह" Z "ळ" Z "ऴ" Z "ऱ" Z "ऩ" Z "क़" Z "ख़" Z "ग़" Z "ज़" Z "ड़" Z "ढ़" Z "फ़" Z "य़" Z "ँˆग" Z "ँˆज" Z "ँˆड" Z "ँˆद" Z "ँˆब" Z E
    OTHERDIACRITICS "ँ" Z "ं" Z "ः" Z "ःʼ" Z E
    SYMBOLS "०" Z "१" Z "२" Z "३" Z "४" Z "५" Z "६" Z "७" Z "८" Z "९" Z "ॐ" Z "।" Z "॥" Z E
    VEDICSYMBOLS "ऽ" Z "ꣳ" Z "ꣴ" Z E
    LANGUAGES
      "hindi" Z E
    E
    EQUIVALENTS
      "ॐ" Z "ओ३म्" Z E
      "क़" Z "क़" Z E
      "ख़" Z "ख़" Z E
      "ग़" Z "ग़" Z E
      "ज़" Z "ज़" Z E
      "ड़" Z "ड़" Z E
      "ढ़" Z "ढ़" Z E
      "फ़" Z "फ़" Z E
      "य़" Z "य़" Z E
      "्" Z "᳭" Z "्‍" Z "‍्" Z E
      "ऽ" Z "ᳵ" Z "ᳶ" Z E
      "ऍ" Z "ॲ" Z "अॅ" Z E
    E
  E
  "gujarati" Z VEDIC
    VOWELS "અ" Z "આ" Z "ઇ" Z "ઈ" Z "ઉ" Z "ઊ" Z "ઋ" Z "ૠ" Z "ઌ" Z "ૡ" Z "એ᳴" Z "એ" Z "ઐ" Z "ઓ᳴" Z "ઓ" Z "ઔ" Z "ઍ" Z "ઍˇ" Z "ઑ" Z E
    VOWELMARKS "્" Z "ા" Z "િ" Z "ી" Z "ુ" Z "ૂ" Z "ૃ" Z "ૄ" Z "ૢ" Z "ૣ" Z "ે᳴" Z "ે" Z "ૈ" Z "ો᳴" Z "ો" Z "ૌ" Z "ૅ" Z "ૅˇ" Z "ૉ" Z E
    CONSONANTS "ક" Z "ખ" Z "ગ" Z "ઘ" Z "ઙ" Z "ચ" Z "છ" Z "જ" Z "ઝ" Z "ઞ" Z "ટ" Z "ઠ" Z "ડ" Z "ઢ" Z "ણ" Z "ત" Z "થ" Z "દ" Z "ધ" Z "ન" Z "પ" Z "ફ" Z "બ" Z "ભ" Z "મ" Z "ય" Z "ર" Z "લ" Z "વ" Z "શ" Z "ષ" Z "સ" Z "હ" Z "ળ" Z "ળ઼" Z "ર઼" Z "ન઼" Z "ક઼" Z "ખ઼" Z "ગ઼" Z "જ઼" Z "ડ઼" Z "ઢ઼" Z "ફ઼" Z "ય઼" Z "ઁˆગ" Z "ઁˆજ" Z "ઁˆડ" Z "ઁˆદ" Z "ઁˆબ" Z E
    OTHERDIACRITICS "ઁ" Z "ં" Z "ઃ" Z "ઃʼ" Z E
    SYMBOLS "૦" Z "૧" Z "૨" Z "૩" Z "૪" Z "૫" Z "૬" Z "૭" Z "૮" Z "૯" Z "ૐ" Z "।" Z "॥" Z E
    VEDICSYMBOLS "ઽ" Z "ꣳ" Z "ꣴ" Z E
  E
  "gurmukhi" Z VEDIC
    ALIASES "punjabi" Z E
    VOWELS "ਅ" Z "ਆ" Z "ਇ" Z "ਈ" Z "ਉ" Z "ਊ" Z "ਰੁʼ" Z "ਰੂʼ" Z "ਲੁʼ" Z "ਲੂʼ" Z "ਏ᳴" Z "ਏ" Z "ਐ" Z "ਓ᳴" Z "ਓ" Z "ਔ" Z "ਏ᳴" Z "ਏˇ" Z "ਆ᳴" Z E
    VOWELMARKS "੍" Z "ਾ" Z "ਿ" Z "ੀ" Z "ੁ" Z "ੂ" Z "੍ਰੁʼ" Z "੍ਰੂʼ" Z "੍ਲੁʼ" Z "੍ਲੂʼ" Z "ੇ᳴" Z "ੇ" Z "ੈ" Z "ੋ᳴" Z "ੋ" Z "ੌ" Z "ੇ᳴" Z "ੇˇ" Z "ਾ᳴" Z E
    CONSONANTS "ਕ" Z "ਖ" Z "ਗ" Z "ਘ" Z "ਙ" Z "ਚ" Z "ਛ" Z "ਜ" Z "ਝ" Z "ਞ" Z "ਟ" Z "ਠ" Z "ਡ" Z "ਢ" Z "ਣ" Z "ਤ" Z "ਥ" Z "ਦ" Z "ਧ" Z "ਨ" Z "ਪ" Z "ਫ" Z "ਬ" Z "ਭ" Z "ਮ" Z "ਯ" Z "ਰ" Z "ਲ" Z "ਵ" Z "ਸ਼" Z "ਸ਼਼" Z "ਸ" Z "ਹ" Z "ਲ਼" Z "ਲ਼਼" Z "ਰ਼" Z "ਨ਼" Z "ਕ਼" Z "ਖ਼" Z "ਗ਼" Z "ਜ਼" Z "ੜ" Z "ਢ਼" Z "ਫ਼" Z "ਯ਼" Z "ਁˆਗ" Z "ਁˆਜ" Z "ਁˆਡ" Z "ਁˆਦ" Z "ਁˆਬ" Z E
    OTHERDIACRITICS "ਁ" Z "ਂ" Z "ਃ" Z "ਃʼ" Z E
    SYMBOLS "੦" Z "੧" Z "੨" Z "੩" Z "੪" Z "੫" Z "੬" Z "੭" Z "੮" Z "੯" Z "ੴ" Z "।" Z "॥" Z E
    VEDICSYMBOLS "(ਅ)" Z "ꣳ" Z "ꣴ" Z E
    EQUIVALENTS
      "ਂ" Z "ੰ" Z E
    E
  E
  "iast" Z LATIN
    VOWELS "a" Z "ā" Z "i" Z "ī" Z "u" Z "ū" Z "ṛ" Z "ṝ" Z "ḷ" Z "ḹ" Z "ĕ" Z "e" Z "ai" Z "ŏ" Z "o" Z "au" Z "æ" Z "ǣ" Z "ô" Z E
    VOWELMARKS "" Z "ā" Z "i" Z "ī" Z "u" Z "ū" Z "ṛ" Z "ṝ" Z "ḷ" Z "ḹ" Z "ĕ" Z "e" Z "ai" Z "ŏ" Z "o" Z "au" Z "æ" Z "ǣ" Z "ô" Z E
    CONSONANTS "k" Z "kh" Z "g" Z "gh" Z "ṅ" Z "c" Z "ch" Z "j" Z "jh" Z "ñ" Z "ṭ" Z "ṭh" Z "ḍ" Z "ḍh" Z "ṇ" Z "t" Z "th" Z "d" Z "dh" Z "n" Z "p" Z "ph" Z "b" Z "bh" Z "m" Z "y" Z "r" Z "l" Z "v" Z "ś" Z "ṣ" Z "s" Z "h" Z "l̤" Z "ḻ" Z "ṟ" Z "ṉ" Z "q" Z "k͟h" Z "ġ" Z "z" Z "r̤" Z "r̤h" Z "f" Z "ẏ" Z "n̆g" Z "n̆j" Z "n̆ḍ" Z "n̆d" Z "m̆b" Z E
    OTHERDIACRITICS "m̐" Z "ṃ" Z "ḥ" Z "ḵ" Z E
    SYMBOLS "0" Z "1" Z "2" Z "3" Z "4" Z "5" Z "6" Z "7" Z "8" Z "9" Z "oṃ" Z "|" Z "||" Z E
    VEDICSYMBOLS "'" Z "gͫ" Z "gͫ̄" Z E
    EQUIVALENTS
      "||" Z "//" Z ".." Z E
      "|" Z "/" Z "." Z E
      "x:0" Z "()" Z "^" Z E
      "x:1" Z "{}" Z "^^" Z E
      "ṛ" Z "r̥" Z "Ṛ" Z "ṛ" Z "Ṛ" Z E
      "a" Z "A" Z E
      "ā" Z "Ā" Z "ā" Z "Ā" Z E
      "i" Z "I" Z E
      "ī" Z "Ī" Z "ī" Z "Ī" Z E
      "u" Z "U" Z E
      "ū" Z "Ū" Z "ū" Z "Ū" Z E
      "ṝ" Z "Ṝ" Z "ṝ" Z "Ṝ" Z E
      "ḷ" Z "Ḷ" Z "ḷ" Z "Ḷ" Z E
      "ḹ" Z "Ḹ" Z "ḹ" Z "Ḹ" Z E
      "ĕ" Z "Ĕ" Z "ĕ" Z "Ĕ" Z E
      "e" Z "E" Z E
      "ai" Z "AI" Z E
      "ŏ" Z "Ŏ" Z "ŏ" Z "Ŏ" Z E
      "o" Z "O" Z E
      "au" Z "AU" Z E
      "æ" Z "Æ" Z E
      "ǣ" Z "Ǣ" Z E
      "ô" Z "Ô" Z "ô" Z "Ô" Z E
      "k" Z "K" Z E
      "kh" Z "KH" Z E
      "g" Z "G" Z E
      "gh" Z "GH" Z E
      "ṅ" Z "Ṅ" Z "ṅ" Z "Ṅ" Z E
      "c" Z "C" Z E
      "ch" Z "CH" Z E
      "j" Z "J" Z E
      "jh" Z "JH" Z E
      "ñ" Z "Ñ" Z "ñ" Z "Ñ" Z E
      "ṭ" Z "Ṭ" Z "ṭ" Z "Ṭ" Z E
      "ṭh" Z "ṬH" Z "ṭh" Z "ṬH" Z E
      "ḍ" Z "Ḍ" Z "ḍ" Z "Ḍ" Z E
      "ḍh" Z "ḌH" Z "ḍh" Z "ḌH" Z E
      "ṇ" Z "Ṇ" Z "ṇ" Z "Ṇ" Z E
      "t" Z "T" Z E
      "th" Z "TH" Z E
      "d" Z "D" Z E
      "dh" Z "DH" Z E
      "n" Z "N" Z E
      "p" Z "P" Z E
      "ph" Z "PH" Z E
      "b" Z "B" Z E
      "bh" Z "BH" Z E
      "m" Z "M" Z E
      "y" Z "Y" Z E
      "r" Z "R" Z E
      "l" Z "L" Z E
      "v" Z "V" Z E
      "ś" Z "Ś" Z "ś" Z "Ś" Z E
      "ṣ" Z "Ṣ" Z "ṣ" Z "Ṣ" Z E
      "s" Z "S" Z E
      "h" Z "H" Z E
      "l̤" Z "L̤" Z E
      "ḻ" Z "Ḻ" Z E
      "ṟ" Z "Ṟ" Z E
      "ṉ" Z "Ṉ" Z E
      "q" Z "Q" Z E
      "k͟h" Z "K͟H" Z E
      "ġ" Z "Ġ" Z "ġ" Z "Ġ" Z E
      "z" Z "Z" Z E
      "r̤" Z "R̤" Z E
      "r̤h" Z "R̤H" Z E
      "f" Z "F" Z E
      "ẏ" Z "Ẏ" Z "ẏ" Z "Ẏ" Z E
      "n̆g" Z "N̆G" Z E
      "n̆j" Z "N̆J" Z E
      "n̆ḍ" Z "N̆Ḍ" Z "n̆ḍ" Z "N̆Ḍ" Z E
      "n̆d" Z "N̆D" Z E
      "m̆b" Z "M̆B" Z E
      "m̐" Z "M̐" Z E
      "ṃ" Z "Ṃ" Z "ṃ" Z "Ṃ" Z E
      "ḥ" Z "Ḥ" Z "ḥ" Z "Ḥ" Z E
      "ḵ" Z "Ḵ" Z E
      "oṃ" Z "OṂ" Z "oṃ" Z "OṂ" Z E
      "gͫ" Z "Gͫ" Z E
      "gͫ̄" Z "Gͫ̄" Z E
    E
  E
  "ipa" Z LATIN
    VOWELS "ə" Z "ɑː" Z "ɪ" Z "iː" Z "u" Z "uː" Z "ɾɨ" Z "ɾɨː" Z "lɨ" Z "lɨː" Z "e" Z "eː" Z "əɪ" Z "o" Z "oː" Z "əu" Z "æ" Z "æː" Z "ɔ" Z E
    VOWELMARKS "" Z "ɑː" Z "ɪ" Z "iː" Z "u" Z "uː" Z "ɾɨ" Z "ɾɨː" Z "lɨ" Z "lɨː" Z "e" Z "eː" Z "əɪ" Z "o" Z "oː" Z "əu" Z "æ" Z "æː" Z "ɔ" Z E
    CONSONANTS "k" Z "kʰ" Z "g" Z "gʰ" Z "ŋ" Z "t͡ʃ" Z "t͡ʃʰ" Z "d͡ʒ" Z "d͡ʒʰ" Z "ɲ" Z "ʈ" Z "ʈʰ" Z "ɖ" Z "ɖʰ" Z "ɳ" Z "t̪" Z "t̪ʰ" Z "d̪" Z "d̪ʰ" Z "n̪" Z "p" Z "pʰ" Z "b" Z "bʰ" Z "m" Z "j" Z "ɾ" Z "l" Z "ʋ" Z "ʃ" Z "ʂ" Z "s̪" Z "ɦ" Z "ɭ" Z "ɻ" Z "r" Z "n" Z "q" Z "x" Z "ɣ" Z "z" Z "ɽ" Z "ɽʰ" Z "f" Z "ẏ" Z "ᵑɡ" Z "ᶮd͡ʒ" Z "ᶯɖ" Z "ⁿd" Z "ᵐb" Z E
    OTHERDIACRITICS "̃ʽ" Z "̃" Z "h" Z "xʽ" Z E
    SYMBOLS "0" Z "1" Z "2" Z "3" Z "4" Z "5" Z "6" Z "7" Z "8" Z "9" Z "oːm" Z "|" Z "||" Z E
    VEDICSYMBOLS "ə̸" Z "gͫ" Z "gͫ̄" Z E
    EQUIVALENTS
      "||" Z "//" Z ".." Z E
      "|" Z "/" Z "." Z E
      "x:0" Z "()" Z "^" Z E
      "x:1" Z "{}" Z "^^" Z E
      "ɑː" Z "aː" Z "Ɑː" Z E
      "ə" Z "Ə" Z E
      "iː" Z "Iː" Z E
      "u" Z "U" Z E
      "uː" Z "Uː" Z E
      "ɾɨ" Z "ɾƗ" Z E
      "ɾɨː" Z "ɾƗː" Z E
      "lɨ" Z "LƗ" Z E
      "lɨː" Z "LƗː" Z E
      "e" Z "E" Z E
      "eː" Z "Eː" Z E
      "əɪ" Z "Əɪ" Z E
      "o" Z "O" Z E
      "oː" Z "Oː" Z E
      "əu" Z "ƏU" Z E
      "æ" Z "Æ" Z E
      "æː" Z "Æː" Z E
      "ɔ" Z "Ɔ" Z E
      "k" Z "K" Z E
      "kʰ" Z "Kʰ" Z E
      "g" Z "G" Z E
      "gʰ" Z "Gʰ" Z E
      "ŋ" Z "Ŋ" Z E
      "t͡ʃ" Z "T͡Ʃ" Z E
      "t͡ʃʰ" Z "T͡Ʃʰ" Z E
      "d͡ʒ" Z "D͡Ʒ" Z E
      "d͡ʒʰ" Z "D͡Ʒʰ" Z E
      "ɲ" Z "Ɲ" Z E
      "ʈ" Z "Ʈ" Z E
      "ʈʰ" Z "Ʈʰ" Z E
      "ɖ" Z "Ɖ" Z E
      "ɖʰ" Z "Ɖʰ" Z E
      "t̪" Z "T̪" Z E
      "t̪ʰ" Z "T̪ʰ" Z E
      "d̪" Z "D̪" Z E
      "d̪ʰ" Z "D̪ʰ" Z E
      "n̪" Z "N̪" Z E
      "p" Z "P" Z E
      "pʰ" Z "Pʰ" Z E
      "b" Z "B" Z E
      "bʰ" Z "Bʰ" Z E
      "m" Z "M" Z E
      "j" Z "J" Z E
      "l" Z "L" Z E
      "ʋ" Z "Ʋ" Z E
      "ʃ" Z "Ʃ" Z E
      "s̪" Z "S̪" Z E
      "r" Z "R" Z E
      "n" Z "N" Z E
      "q" Z "Q" Z E
      "x" Z "X" Z E
      "ɣ" Z "Ɣ" Z E
      "z" Z "Z" Z E
      "ɽ" Z "Ɽ" Z E
      "ɽʰ" Z "Ɽʰ" Z E
      "f" Z "F" Z E
      "ẏ" Z "Ẏ" Z "ẏ" Z "Ẏ" Z E
      "ᶮd͡ʒ" Z "ᶮD͡Ʒ" Z E
      "ᶯɖ" Z "ᶯƉ" Z E
      "ⁿd" Z "ⁿD" Z E
      "ᵐb" Z "ᵐB" Z E
      "h" Z "H" Z E
      "xʽ" Z "Xʽ" Z E
      "oːm" Z "OːM" Z E
      "ə̸" Z "Ə̸" Z E
      "gͫ" Z "Gͫ" Z E
      "gͫ̄" Z "Gͫ̄" Z E
    E
  E
  "iso" Z LATIN
    VOWELS "a" Z "ā" Z "i" Z "ī" Z "u" Z "ū" Z "r̥" Z "r̥̄" Z "l̥" Z "l̥̄" Z "e" Z "ē" Z "ai" Z "o" Z "ō" Z "au" Z "æ" Z "ǣ" Z "ô" Z E
    VOWELMARKS "" Z "ā" Z "i" Z "ī" Z "u" Z "ū" Z "r̥" Z "r̥̄" Z "l̥" Z "l̥̄" Z "e" Z "ē" Z "ai" Z "o" Z "ō" Z "au" Z "æ" Z "ǣ" Z "ô" Z E
    CONSONANTS "k" Z "kh" Z "g" Z "gh" Z "ṅ" Z "c" Z "ch" Z "j" Z "jh" Z "ñ" Z "ṭ" Z "ṭh" Z "ḍ" Z "ḍh" Z "ṇ" Z "t" Z "th" Z "d" Z "dh" Z "n" Z "p" Z "ph" Z "b" Z "bh" Z "m" Z "y" Z "r" Z "l" Z "v" Z "ś" Z "ṣ" Z "s" Z "h" Z "ḷ" Z "ḻ" Z "ṟ" Z "ṉ" Z "q" Z "k͟h" Z "ġ" Z "z" Z "ṛ" Z "ṛh" Z "f" Z "ẏ" Z "n̆g" Z "n̆j" Z "n̆ḍ" Z "n̆d" Z "m̆b" Z E
    OTHERDIACRITICS "m̐" Z "ṁ" Z "ḥ" Z "ḵ" Z E
    SYMBOLS "0" Z "1" Z "2" Z "3" Z "4" Z "5" Z "6" Z "7" Z "8" Z "9" Z "ōṁ" Z "|" Z "||" Z E
    VEDICSYMBOLS "’" Z "gͫ" Z "gͫ̄" Z E
    EQUIVALENTS
      "||" Z "//" Z ".." Z E
      "|" Z "/" Z "." Z E
      "x:0" Z "()" Z "^" Z E
      "x:1" Z "{}" Z "^^" Z E
      "a" Z "A" Z E
      "ā" Z "Ā" Z "ā" Z "Ā" Z E
      "i" Z "I" Z E
      "ī" Z "Ī" Z "ī" Z "Ī" Z E
      "u" Z "U" Z E
      "ū" Z "Ū" Z "ū" Z "Ū" Z E
      "r̥" Z "R̥" Z E
      "r̥̄" Z "R̥̄" Z E
      "l̥" Z "L̥" Z E
      "l̥̄" Z "L̥̄" Z E
      "e" Z "E" Z E
      "ē" Z "Ē" Z "ē" Z "Ē" Z E
      "ai" Z "AI" Z E
      "o" Z "O" Z E
      "ō" Z "Ō" Z "ō" Z "Ō" Z E
      "au" Z "AU" Z E
      "æ" Z "Æ" Z E
      "ǣ" Z "Ǣ" Z E
      "ô" Z "Ô" Z "ô" Z "Ô" Z E
      "k" Z "K" Z E
      "kh" Z "KH" Z E
      "g" Z "G" Z E
      "gh" Z "GH" Z E
      "ṅ" Z "Ṅ" Z "ṅ" Z "Ṅ" Z E
      "c" Z "C" Z E
      "ch" Z "CH" Z E
      "j" Z "J" Z E
      "jh" Z "JH" Z E
      "ñ" Z "Ñ" Z "ñ" Z "Ñ" Z E
      "ṭ" Z "Ṭ" Z "ṭ" Z "Ṭ" Z E
      "ṭh" Z "ṬH" Z "ṭh" Z "ṬH" Z E
      "ḍ" Z "Ḍ" Z "ḍ" Z "Ḍ" Z E
      "ḍh" Z "ḌH" Z "ḍh" Z "ḌH" Z E
      "ṇ" Z "Ṇ" Z "ṇ" Z "Ṇ" Z E
      "t" Z "T" Z E
      "th" Z "TH" Z E
      "d" Z "D" Z E
      "dh" Z "DH" Z E
      "n" Z "N" Z E
      "p" Z "P" Z E
      "ph" Z "PH" Z E
      "b" Z "B" Z E
      "bh" Z "BH" Z E
      "m" Z "M" Z E
      "y" Z "Y" Z E
      "r" Z "R" Z E
      "l" Z "L" Z E
      "v" Z "V" Z E
      "ś" Z "Ś" Z "ś" Z "Ś" Z E
      "ṣ" Z "Ṣ" Z "ṣ" Z "Ṣ" Z E
      "s" Z "S" Z E
      "h" Z "H" Z E
      "ḷ" Z "Ḷ" Z "ḷ" Z "Ḷ" Z E
      "ḻ" Z "Ḻ" Z E
      "ṟ" Z "Ṟ" Z E
      "ṉ" Z "Ṉ" Z E
      "q" Z "Q" Z E
      "k͟h" Z "K͟H" Z E
      "ġ" Z "Ġ" Z "ġ" Z "Ġ" Z E
      "z" Z "Z" Z E
      "ṛ" Z "Ṛ" Z "ṛ" Z "Ṛ" Z E
      "ṛh" Z "ṚH" Z "ṛh" Z "ṚH" Z E
      "f" Z "F" Z E
      "ẏ" Z "Ẏ" Z "ẏ" Z "Ẏ" Z E
      "n̆g" Z "N̆G" Z E
      "n̆j" Z "N̆J" Z E
      "n̆ḍ" Z "N̆Ḍ" Z "n̆ḍ" Z "N̆Ḍ" Z E
      "n̆d" Z "N̆D" Z E
      "m̆b" Z "M̆B" Z E
      "m̐" Z "M̐" Z E
      "ṁ" Z "Ṁ" Z "ṁ" Z "Ṁ" Z E
      "ḥ" Z "Ḥ" Z "ḥ" Z "Ḥ" Z E
      "ḵ" Z "Ḵ" Z E
      "ōṁ" Z "ŌṀ" Z "ōṁ" Z "ŌṀ" Z E
      "gͫ" Z "Gͫ" Z E
      "gͫ̄" Z "Gͫ̄" Z E
    E
  E
  "itrans" Z LATIN
    VOWELS "a" Z "aa" Z "i" Z "ee" Z "u" Z "uu" Z "R^i" Z "R^I" Z "L^i" Z "L^I" Z "^e" Z "e" Z "ai" Z "^o" Z "o" Z "au" Z "e.c" Z "a.C" Z "A.c" Z E
    VOWELMARKS "" Z "aa" Z "i" Z "ee" Z "u" Z "uu" Z "R^i" Z "R^I" Z "L^i" Z "L^I" Z "^e" Z "e" Z "ai" Z "^o" Z "o" Z "au" Z "e.c" Z "a.C" Z "A.c" Z E
    CONSONANTS "k" Z "kh" Z "g" Z "gh" Z "~N" Z "ch" Z "chh" Z "j" Z "jh" Z "~n" Z "T" Z "Th" Z "D" Z "Dh" Z "N" Z "t" Z "th" Z "d" Z "dh" Z "n" Z "p" Z "ph" Z "b" Z "bh" Z "m" Z "y" Z "r" Z "l" Z "v" Z "sh" Z "Sh" Z "s" Z "h" Z "L" Z "zh" Z "R" Z "^n" Z "q" Z "K" Z "G" Z "z" Z ".D" Z ".Dh" Z "f" Z "Y" Z "n*g" Z "n*j" Z "n*D" Z "n*d" Z "m*b" Z E
    OTHERDIACRITICS ".N" Z "M" Z "H" Z "K^" Z E
    SYMBOLS "0" Z "1" Z "2" Z "3" Z "4" Z "5" Z "6" Z "7" Z "8" Z "9" Z "OM" Z "|" Z "||" Z E
    VEDICSYMBOLS ".a" Z "gum" Z "ggum" Z E
    EQUIVALENTS
      "aa" Z "A" Z E
      "ee" Z "I" Z "ii" Z E
      "uu" Z "U" Z "oo" Z E
      "R^i" Z "RRi" Z E
      "R^I" Z "RRI" Z E
      "L^i" Z "LLi" Z E
      "L^I" Z "LLI" Z E
      "e" Z "E" Z E
      "o" Z "O" Z E
      "e.c" Z "c.e" Z E
      "A.c" Z "c.o" Z E
      "chh" Z "Ch" Z E
      "L" Z "ld" Z E
      "z" Z "J" Z E
      ".n" Z "M" Z ".m" Z E
      "||" Z "//" Z ".." Z E
      "|" Z "/" Z "." Z E
      "x:0" Z "()" Z "^" Z E
      "x:1" Z "{}" Z "^^" Z "+" Z E
      "x:2" Z "_" Z E
      "OM" Z "aum" Z "AUM" Z "oM" Z E
      "shree" Z "SRI" Z E
      "kSh" Z "x" Z E
      "j~n" Z "GY" Z "dny" Z E
    E
  E
  "kannada" Z VEDIC
    VOWELS "ಅ" Z "ಆ" Z "ಇ" Z "ಈ" Z "ಉ" Z "ಊ" Z "ಋ" Z "ೠ" Z "ಌ" Z "ೡ" Z "ಎ" Z "ಏ" Z "ಐ" Z "ಒ" Z "ಓ" Z "ಔ" Z "ಎ᳴" Z "ಏˇ" Z "ಆ᳴" Z E
    VOWELMARKS "್" Z "ಾ" Z "ಿ" Z "ೀ" Z "ು" Z "ೂ" Z "ೃ" Z "ೄ" Z "ೢ" Z "ೣ" Z "ೆ" Z "ೇ" Z "ೈ" Z "ೊ" Z "ೋ" Z "ೌ" Z "ೆ᳴" Z "ೇˇ" Z "ಾ᳴" Z E
    CONSONANTS "ಕ" Z "ಖ" Z "ಗ" Z "ಘ" Z "ಙ" Z "ಚ" Z "ಛ" Z "ಜ" Z "ಝ" Z "ಞ" Z "ಟ" Z "ಠ" Z "ಡ" Z "ಢ" Z "ಣ" Z "ತ" Z "ಥ" Z "ದ" Z "ಧ" Z "ನ" Z "ಪ" Z "ಫ" Z "ಬ" Z "ಭ" Z "ಮ" Z "ಯ" Z "ರ" Z "ಲ" Z "ವ" Z "ಶ" Z "ಷ" Z "ಸ" Z "ಹ" Z "ಳ" Z "ೞ" Z "ಱ" Z "ನ಼" Z "ಕ಼" Z "ಖ಼" Z "ಗ಼" Z "ಜ಼" Z "ಡ಼" Z "ಢ಼" Z "ಫ಼" Z "ಯ಼" Z "ಂˆಗ" Z "ಂˆಜ" Z "ಂˆಡ" Z "ಂˆದ" Z "ಂˆಬ" Z E
    OTHERDIACRITICS "ಁ" Z "ಂ" Z "ಃ" Z "ಃʼ" Z E
    SYMBOLS "೦" Z "೧" Z "೨" Z "೩" Z "೪" Z "೫" Z "೬" Z "೭" Z "೮" Z "೯" Z "ಓಂ" Z "।" Z "॥" Z E
    VEDICSYMBOLS "ಽ" Z "ꣳ" Z "ꣴ" Z E
    EQUIVALENTS
      "್" Z "್‍" Z "‍್" Z E
      "ಽ" Z "ೱ" Z "ೲ" Z E
    E
  E
  "khmer" Z INDIC
    VOWELS "អ" Z "អា" Z "ឥ" Z "ឦ" Z "ឧ" Z "ឩ" Z "ឫ" Z "ឬ" Z "ឭ" Z "ឮ" Z "ឯʽ" Z "ឯ" Z "ឰ" Z "ឱʽ" Z "ឱ" Z "ឳ" Z "ឯʽ" Z "ឯʽ" Z "អាʽ" Z E
    VOWELMARKS "៑" Z "ា" Z "ិ" Z "ី" Z "ុ" Z "ូ" Z "្ឫ" Z "្ឬ" Z "្ឭ" Z "្ឮ" Z "េʽ" Z "េ" Z "ៃ" Z "ោʽ" Z "ោ" Z "ៅ" Z "េʽ" Z "េʽ" Z "ាʽ" Z E
    CONSONANTS "ក" Z "ខ" Z "គ" Z "ឃ" Z "ង" Z "ច" Z "ឆ" Z "ជ" Z "ឈ" Z "ញ" Z "ដ" Z "ឋ" Z "ឌ" Z "ឍ" Z "ណ" Z "ត" Z "ថ" Z "ទ" Z "ធ" Z "ន" Z "ប" Z "ផ" Z "ព" Z "ភ" Z "ម" Z "យ" Z "រ" Z "ល" Z "វ" Z "ឝ" Z "ឞ" Z "ស" Z "ហ" Z "ឡ" Z "ឡʽ" Z "រʽ" Z "នʽ" Z "កʽ" Z "ខʽ" Z "គʽ" Z "ជʽ" Z "ឌʽ" Z "ឍʽ" Z "ផʽ" Z "យʽ" Z "ំគʽ" Z "ំជʽ" Z "ំឌʽ" Z "ំទʽ" Z "ំពʽ" Z E
    OTHERDIACRITICS "ំʽ" Z "ំ" Z "ះ" Z "ះʽ" Z E
    SYMBOLS "០" Z "១" Z "២" Z "៣" Z "៤" Z "៥" Z "៦" Z "៧" Z "៨" Z "៩" Z "ឱំ" Z "។" Z "៕" Z E
    VEDICSYMBOLS "ៜ" Z "ꣳ" Z "ꣴ" Z E
  E
  "malayalam" Z VEDIC
    VOWELS "അ" Z "ആ" Z "ഇ" Z "ഈ" Z "ഉ" Z "ഊ" Z "ഋ" Z "ൠ" Z "ഌ" Z "ൡ" Z "എ" Z "ഏ" Z "ഐ" Z "ഒ" Z "ഓ" Z "ഔ" Z "എ᳴" Z "ഏˇ" Z "ആ᳴" Z E
    VOWELMARKS "്" Z "ാ" Z "ി" Z "ീ" Z "ു" Z "ൂ" Z "ൃ" Z "ൄ" Z "ൢ" Z "ൣ" Z "െ" Z "േ" Z "ൈ" Z "ൊ" Z "ോ" Z "ൗ" Z "െ᳴" Z "േˇ" Z "ാ᳴" Z E
    CONSONANTS "ക" Z "ഖ" Z "ഗ" Z "ഘ" Z "ങ" Z "ച" Z "ഛ" Z "ജ" Z "ഝ" Z "ഞ" Z "ട" Z "ഠ" Z "ഡ" Z "ഢ" Z "ണ" Z "ത" Z "ഥ" Z "ദ" Z "ധ" Z "ന" Z "പ" Z "ഫ" Z "ബ" Z "ഭ" Z "മ" Z "യ" Z "ര" Z "ല" Z "വ" Z "ശ" Z "ഷ" Z "സ" Z "ഹ" Z "ള" Z "ഴ" Z "റ" Z "ഩ" Z "ക·" Z "ഖ·" Z "ഗ·" Z "ജ·" Z "ഡ·" Z "ഢ·" Z "ഫ·" Z "യ·" Z "ംˆഗ" Z "ംˆജ" Z "ംˆഡ" Z "ംˆദ" Z "ംˆബ" Z E
    OTHERDIACRITICS "ഁ" Z "ം" Z "ഃ" Z "ഃʼ" Z E
    SYMBOLS "൦" Z "൧" Z "൨" Z "൩" Z "൪" Z "൫" Z "൬" Z "൭" Z "൮" Z "൯" Z "ഓം" Z "।" Z "॥" Z E
    VEDICSYMBOLS "ഽ" Z "ꣳ" Z "ꣴ" Z E
    EQUIVALENTS
      "ൌ" Z "ൗ" Z E
      "്" Z "്‍" Z "‍്" Z E
      "ണ്" Z "ൺ" Z E
      "ന്" Z "ൻ" Z E
      "ര്" Z "ർ" Z E
      "ല്" Z "ൽ" Z E
      "ള്" Z "ൾ" Z E
      "ക്" Z "ൿ" Z E
      "മ്" Z "ൔ" Z E
      "യ്" Z "ൕ" Z E
      "ഴ്" Z "ൖ" Z E
    E
  E
  "oriya" Z VEDIC
    ALIASES "odia" Z E
    VOWELS "ଅ" Z "ଆ" Z "ଇ" Z "ଈ" Z "ଉ" Z "ଊ" Z "ଋ" Z "ୠ" Z "ଌ" Z "ୡ" Z "ଏ᳴" Z "ଏ" Z "ଐ" Z "ଓ᳴" Z "ଓ" Z "ଔ" Z "ଏ᳴" Z "ଏˇ" Z "ଆ᳴" Z E
    VOWELMARKS "୍" Z "ା" Z "ି" Z "ୀ" Z "ୁ" Z "ୂ" Z "ୃ" Z "ୄ" Z "ୢ" Z "ୣ" Z "େ᳴" Z "େ" Z "ୈ" Z "ୋ᳴" Z "ୋ" Z "ୌ" Z "େ᳴" Z "େˇ" Z "ା᳴" Z E
    CONSONANTS "କ" Z "ଖ" Z "ଗ" Z "ଘ" Z "ଙ" Z "ଚ" Z "ଛ" Z "ଜ" Z "ଝ" Z "ଞ" Z "ଟ" Z "ଠ" Z "ଡ" Z "ଢ" Z "ଣ" Z "ତ" Z "ଥ" Z "ଦ" Z "ଧ" Z "ନ" Z "ପ" Z "ଫ" Z "ବ" Z "ଭ" Z "ମ" Z "ଯ" Z "ର" Z "ଲ" Z "ୱ" Z "ଶ" Z "ଷ" Z "ସ" Z "ହ" Z "ଳ" Z "ଳ଼" Z "ର଼" Z "ନ଼" Z "କ଼" Z "ଖ଼" Z "ଗ଼" Z "ଜ଼" Z "ଡ଼" Z "ଢ଼" Z "ଫ଼" Z "ୟ" Z "ଁˆଗ" Z "ଁˆଜ" Z "ଁˆଡ" Z "ଁˆଦ" Z "ଁˆବ" Z E
    OTHERDIACRITICS "ଁ" Z "ଂ" Z "ଃ" Z "ଃʼ" Z E
    SYMBOLS "୦" Z "୧" Z "୨" Z "୩" Z "୪" Z "୫" Z "୬" Z "୭" Z "୮" Z "୯" Z "ଓଁ" Z "।" Z "॥" Z E
    VEDICSYMBOLS "ଽ" Z "ꣳ" Z "ꣴ" Z E
    EQUIVALENTS
      "ୱ" Z "ଵ" Z "ଵ" Z "ବ଼" Z E
    E
  E
  "readableLatin" Z LATIN
    VOWELS "a" Z "aa" Z "i" Z "ee" Z "u" Z "oo" Z "r̥" Z "rr̥" Z "l̥" Z "ll̥" Z "e" Z "E" Z "ai" Z "o" Z "O" Z "au" Z "a" Z "e" Z "o" Z E
    VOWELMARKS "" Z "aa" Z "i" Z "ee" Z "u" Z "oo" Z "r̥" Z "rr̥" Z "l̥" Z "ll̥" Z "e" Z "E" Z "ai" Z "o" Z "O" Z "au" Z "a" Z "e" Z "o" Z E
    CONSONANTS "k" Z "kh" Z "g" Z "gh" Z "n" Z "ch" Z "chh" Z "j" Z "jh" Z "n" Z "T" Z "Th" Z "D" Z "Dh" Z "N" Z "t" Z "th" Z "d" Z "dh" Z "n" Z "p" Z "ph" Z "b" Z "bh" Z "m" Z "y" Z "r" Z "l" Z "v" Z "sh" Z "sh" Z "s" Z "h" Z "L" Z "zh" Z "R" Z "n" Z "q" Z "qh" Z "gh" Z "z" Z "D" Z "Dh" Z "f" Z "y" Z "ng" Z "nj" Z "nD" Z "nd" Z "mb" Z E
    OTHERDIACRITICS "n̐" Z "" Z "h" Z "q" Z E
    SYMBOLS "0" Z "1" Z "2" Z "3" Z "4" Z "5" Z "6" Z "7" Z "8" Z "9" Z "om" Z "!" Z "!!" Z E
    VEDICSYMBOLS "~" Z "gum" Z "ggum" Z E
  E
  "sinhala" Z INDIC
    VOWELS "අ" Z "ආ" Z "ඉ" Z "ඊ" Z "උ" Z "ඌ" Z "ඍ" Z "ඎ" Z "ඏ" Z "ඐ" Z "එ" Z "ඒ" Z "ඓ" Z "ඔ" Z "ඕ" Z "ඖ" Z "ඇ" Z "ඈ" Z "ආ᳴" Z E
    VOWELMARKS "්" Z "ා" Z "ි" Z "ී" Z "ු" Z "ූ" Z "ෘ" Z "ෲ" Z "ෟ" Z "ෳ" Z "ෙ" Z "ේ" Z "ෛ" Z "ො" Z "ෝ" Z "ෞ" Z "ැ" Z "ෑ" Z "ා᳴" Z E
    CONSONANTS "ක" Z "ඛ" Z "ග" Z "ඝ" Z "ඞ" Z "ච" Z "ඡ" Z "ජ" Z "ඣ" Z "ඤ" Z "ට" Z "ඨ" Z "ඩ" Z "ඪ" Z "ණ" Z "ත" Z "ථ" Z "ද" Z "ධ" Z "න" Z "ප" Z "ඵ" Z "බ" Z "භ" Z "ම" Z "ය" Z "ර" Z "ල" Z "ව" Z "ශ" Z "ෂ" Z "ස" Z "හ" Z "ළ" Z "ළ·" Z "ර·" Z "න·" Z "ක·" Z "ඛ·" Z "ග·" Z "ජ·" Z "ඩ·" Z "ඪ·" Z "ෆ" Z "ය·" Z "ඟ" Z "ඦ" Z "ඬ" Z "ඳ" Z "ඹ" Z E
    OTHERDIACRITICS "ංʼ" Z "ං" Z "ඃ" Z "ඃʼ" Z E
    SYMBOLS "෦" Z "෧" Z "෨" Z "෩" Z "෪" Z "෫" Z "෬" Z "෭" Z "෮" Z "෯" Z "ඕං" Z "।" Z "॥" Z E
    VEDICSYMBOLS "(අ)" Z "ꣳ" Z "ꣴ" Z E
    EQUIVALENTS
      "්" Z "්‍" Z "‍්" Z E
    E
  E
  "tamil" Z TAMIL
    VOWELS "அ" Z "ஆ" Z "இ" Z "ஈ" Z "உ" Z "ஊ" Z "ருʼ" Z "ரூʼ" Z "லுʼ" Z "லூʼ" Z "எ" Z "ஏ" Z "ஐ" Z "ஒ" Z "ஓ" Z "ஔ" Z "எ᳴" Z "ஏ᳴" Z "ஆ᳴" Z E
    VOWELMARKS "்" Z "ா" Z "ி" Z "ீ" Z "ு" Z "ூ" Z "்ருʼ" Z "்ரூʼ" Z "்லுʼ" Z "்லூʼ" Z "ெ" Z "ே" Z "ை" Z "ொ" Z "ோ" Z "ௌ" Z "ெ᳴" Z "ே᳴" Z "ா᳴" Z E
    CONSONANTS "க" Z "க²" Z "க³" Z "க⁴" Z "ங" Z "ச" Z "ச²" Z "ஜ" Z "ஜ²" Z "ஞ" Z "ட" Z "ட²" Z "ட³" Z "ட⁴" Z "ண" Z "த" Z "த²" Z "த³" Z "த⁴" Z "ந" Z "ப" Z "ப²" Z "ப³" Z "ப⁴" Z "ம" Z "ய" Z "ர" Z "ல" Z "வ" Z "ஶ" Z "ஷ" Z "ஸ" Z "ஹ" Z "ள" Z "ழ" Z "ற" Z "ன" Z "ஃக" Z "ஃக²" Z "ஃக³" Z "ஃஜ" Z "ஃட²" Z "ஃட³" Z "ஃப" Z "ஃய" Z "ங்ˆக³" Z "ஞ்ˆஜ³" Z "ண்ˆட³" Z "ந்ˆத³" Z "ம்ˆப³" Z E
    OTHERDIACRITICS "னஂ" Z "மஂ" Z "꞉" Z "ஃ" Z E
    SYMBOLS "௦" Z "௧" Z "௨" Z "௩" Z "௪" Z "௫" Z "௬" Z "௭" Z "௮" Z "௯" Z "ௐ" Z "।" Z "॥" Z E
    VEDICSYMBOLS "(அ)" Z "ꣳ" Z "ꣴ" Z E
    EQUIVALENTS
      "ொ" Z "ொ" Z E
      "ோ" Z "ோ" Z E
      "ௌ" Z "ௌ" Z E
      "னஂ" Z "ம்ˮ" Z E
      "மஂ" Z "ம்ʼ" Z E
      "்" Z "்‍" Z "‍்" Z E
    E
  E
  "tamilExtended" Z VEDIC
    VOWELS "അ" Z "ആ" Z "ഇ" Z "ഈ" Z "ഉ" Z "ഊ" Z "ഋ" Z "ൠ" Z "ഌ" Z "ൡ" Z "എ" Z "ഏ" Z "ഐ" Z "ഒ" Z "ഓ" Z "ഔ" Z "എ᳴" Z "ഏ᳴" Z "ആ᳴" Z E
    VOWELMARKS "്‌" Z "ാ" Z "ി" Z "ീ" Z "ു" Z "ൂ" Z "ൃ" Z "ൄ" Z "ൢ" Z "ൣ" Z "െ" Z "േ" Z "ൈ" Z "ൊ" Z "ോ" Z "ൗ" Z "െ᳴" Z "േ᳴" Z "ാ᳴" Z E
    CONSONANTS "ക" Z "ഖ" Z "ഗ" Z "ഘ" Z "ങ" Z "ച" Z "ഛ" Z "ജ" Z "ഝ" Z "ഞ" Z "ട" Z "ഠ" Z "ഡ" Z "ഢ" Z "ണ" Z "ത" Z "ഥ" Z "ദ" Z "ധ" Z "ന" Z "പ" Z "ഫ" Z "ബ" Z "ഭ" Z "മ" Z "യ" Z "ര" Z "ല" Z "വ" Z "ശ" Z "ഷ" Z "സ" Z "ഹ" Z "ള" Z "ഴ" Z "റ" Z "ഩ" Z "ക·" Z "ഖ·" Z "ഗ·" Z "ജ·" Z "ഡ·" Z "ഢ·" Z "ഫ·" Z "യ·" Z "ംˆഗ" Z "ംˆജ" Z "ംˆഡ" Z "ംˆദ" Z "ംˆബ" Z E
    OTHERDIACRITICS "ഁ" Z "ം" Z "ഃ" Z "ஃ" Z E
    SYMBOLS "௦" Z "௧" Z "௨" Z "௩" Z "௪" Z "௫" Z "௬" Z "௭" Z "௮" Z "௯" Z "ௐ" Z "।" Z "॥" Z E
    VEDICSYMBOLS "ഽ" Z "ꣳ" Z "ꣴ" Z E
  E
  "telugu" Z VEDIC
    VOWELS "అ" Z "ఆ" Z "ఇ" Z "ఈ" Z "ఉ" Z "ఊ" Z "ఋ" Z "ౠ" Z "ఌ" Z "ౡ" Z "ఎ" Z "ఏ" Z "ఐ" Z "ఒ" Z "ఓ" Z "ఔ" Z "ఎ᳴​" Z "ఏ᳴" Z "ఒ᳴​" Z E
    VOWELMARKS "్" Z "ా" Z "ి" Z "ీ" Z "ు" Z "ూ" Z "ృ" Z "ౄ" Z "ౢ" Z "ౣ" Z "ె" Z "ే" Z "ై" Z "ొ" Z "ో" Z "ౌ" Z "ె॒​" Z "ే॒​" Z "ొ॒​" Z E
    CONSONANTS "క" Z "ఖ" Z "గ" Z "ఘ" Z "ఙ" Z "చ" Z "ఛ" Z "జ" Z "ఝ" Z "ఞ" Z "ట" Z "ఠ" Z "డ" Z "ఢ" Z "ణ" Z "త" Z "థ" Z "ద" Z "ధ" Z "న" Z "ప" Z "ఫ" Z "బ" Z "భ" Z "మ" Z "య" Z "ర" Z "ల" Z "వ" Z "శ" Z "ష" Z "స" Z "హ" Z "ళ" Z "ఴ" Z "ఱ" Z "న·" Z "క·" Z "ఖ·" Z "గ·" Z "జ·" Z "డ·" Z "ఢ·" Z "ఫ·" Z "య·" Z "ఁˆగ" Z "ఁˆజ" Z "ఁˆడ" Z "ఁˆద" Z "ఁˆబ" Z E
    OTHERDIACRITICS "ఀ" Z "ం" Z "ః" Z "ఃʼ" Z E
    SYMBOLS "౦" Z "౧" Z "౨" Z "౩" Z "౪" Z "౫" Z "౬" Z "౭" Z "౮" Z "౯" Z "ఓం" Z "।" Z "॥" Z E
    VEDICSYMBOLS "ఽ" Z "ꣳ" Z "ꣴ" Z E
    EQUIVALENTS
      "్" Z "్‍" Z "‍్" Z E
    E
  E
  "thai" Z INDIC
    VOWELS "อ" Z "อา" Z "อิ" Z "อี" Z "อุ" Z "อู" Z "ฤ" Z "ฤๅ" Z "ฦ" Z "ฦๅ" Z "อเะ" Z "อเ" Z "อไ" Z "อโะ" Z "อโ" Z "อเา" Z "อแะ" Z "อแ" Z "อเาะ" Z E
    VOWELMARKS "ฺ" Z "า" Z "ิ" Z "ี" Z "ุ" Z "ู" Z "ฺฤ" Z "ฺฤๅ" Z "ฺฦ" Z "ฺฦๅ" Z "เะ" Z "เ" Z "ไ" Z "โะ" Z "โ" Z "เา" Z "แะ" Z "แ" Z "เาะ" Z E
    CONSONANTS "ก" Z "ข" Z "ค" Z "ฆ" Z "ง" Z "จ" Z "ฉ" Z "ช" Z "ฌ" Z "ญ" Z "ฏ" Z "ฐ" Z "ฑ" Z "ฒ" Z "ณ" Z "ต" Z "ถ" Z "ท" Z "ธ" Z "น" Z "ป" Z "ผ" Z "พ" Z "ภ" Z "ม" Z "ย" Z "ร" Z "ล" Z "ว" Z "ศ" Z "ษ" Z "ส" Z "ห" Z "ฬ" Z "ฬʽ" Z "รʽ" Z "นʽ" Z "กʽ" Z "ฃ" Z "ฅ" Z "ซ" Z "รʽ" Z "รฺหʽ" Z "ฟ" Z "ยʽ" Z "ํคʽ" Z "ํชʽ" Z "ํฑʽ" Z "ํทʽ" Z "ํพʽ" Z E
    OTHERDIACRITICS "ํʽ" Z "ํ" Z "ห์" Z "ห์ʽ" Z E
    SYMBOLS "๐" Z "๑" Z "๒" Z "๓" Z "๔" Z "๕" Z "๖" Z "๗" Z "๘" Z "๙" Z "โอํ" Z "ฯ" Z "๚" Z E
    VEDICSYMBOLS "'" Z "ꣳ" Z "ꣴ" Z E
  E
  "tibetan" Z INDIC
    VOWELS "ཨ" Z "ཨཱ" Z "ཨི" Z "ཨཱི" Z "ཨུ" Z "ཨཱུ" Z "རྀ" Z "རཱྀ" Z "ལྀ" Z "ལཱྀ" Z "ཨེʽ" Z "ཨེ" Z "ཨཻ" Z "ཨོʽ" Z "ཨོ" Z "ཨཽ" Z "ཨེʽ" Z "ཨེʽ" Z "ཨཱʽ" Z E
    VOWELMARKS "྄" Z "ཱ" Z "ི" Z "ཱི" Z "ུ" Z "ཱུ" Z "ྲྀ" Z "ྲཱྀ" Z "ླྀ" Z "ླཱྀ" Z "ེʽ" Z "ེ" Z "ཻ" Z "ོʽ" Z "ོ" Z "ཽ" Z "ེʽ" Z "ེʽ" Z "ཱʽ" Z E
    CONSONANTS "ཀ" Z "ཁ" Z "ག" Z "གྷ" Z "ང" Z "ཙ" Z "ཚ" Z "ཛ" Z "ཛྷ" Z "ཉ" Z "ཊ" Z "ཋ" Z "ཌ" Z "ཌྷ" Z "ཎ" Z "ཏ" Z "ཐ" Z "ད" Z "དྷ" Z "ན" Z "པ" Z "ཕ" Z "བ" Z "བྷ" Z "མ" Z "ཡ" Z "ར" Z "ལ" Z "ཝ" Z "ཤ" Z "ཥ" Z "ས" Z "ཧ" Z "ལ༹" Z "ཥ༹" Z "ར༹" Z "ན༹" Z "ཀ༹" Z "ཁ༹" Z "ག༹" Z "ཟ" Z "ཌ༹" Z "ཌྷ༹" Z "ཕ༹" Z "ཡ༹" Z "ྃཀʽ" Z "ྃཛʽ" Z "ྃཌʽ" Z "ྃདʽ" Z "ྃབʽ" Z E
    OTHERDIACRITICS "ྃ" Z "ཾ" Z "ཿ" Z "ཿʽ" Z E
    SYMBOLS "༠" Z "༡" Z "༢" Z "༣" Z "༤" Z "༥" Z "༦" Z "༧" Z "༨" Z "༩" Z "ༀ" Z "།" Z "༎" Z E
    VEDICSYMBOLS "྅" Z "ꣳ" Z "ꣴ" Z E
  E
  "titus" Z LATIN
    VOWELS "a" Z "ā" Z "i" Z "ī" Z "u" Z "ū" Z "r̥" Z "r̥̄" Z "l̥" Z "l̥̄" Z "ĕ" Z "e" Z "ai" Z "ŏ" Z "o" Z "au" Z "æ" Z "ǣ" Z "ô" Z E
    VOWELMARKS "" Z "ā" Z "i" Z "ī" Z "u" Z "ū" Z "r̥" Z "r̥̄" Z "l̥" Z "l̥̄" Z "ĕ" Z "e" Z "ai" Z "ŏ" Z "o" Z "au" Z "æ" Z "ǣ" Z "ô" Z E
    CONSONANTS "k" Z "kʰ" Z "g" Z "gʰ" Z "ṅ" Z "c" Z "cʰ" Z "j" Z "jʰ" Z "ñ" Z "ṭ" Z "ṭʰ" Z "ḍ" Z "ḍʰ" Z "ṇ" Z "t" Z "tʰ" Z "d" Z "dʰ" Z "n" Z "p" Z "pʰ" Z "b" Z "bʰ" Z "m" Z "y" Z "r" Z "l" Z "v" Z "ś" Z "ṣ" Z "s" Z "h" Z "ḷ" Z "ḻ" Z "ṟ" Z "ṉ" Z "q" Z "k͟ʰ" Z "ġ" Z "z" Z "ṛ" Z "ṛʰ" Z "f" Z "ẏ" Z "n̆g" Z "n̆j" Z "n̆ḍ" Z "n̆d" Z "m̆b" Z E
    OTHERDIACRITICS "m̐" Z "ṃ" Z "ḥ" Z "ḵ" Z E
    SYMBOLS "0" Z "1" Z "2" Z "3" Z "4" Z "5" Z "6" Z "7" Z "8" Z "9" Z "oṃ" Z "/" Z "//" Z E
    VEDICSYMBOLS "'" Z "gͫ" Z "gͫ̄" Z E
    EQUIVALENTS
      "//" Z "||" Z ".." Z E
      "/" Z "|" Z "." Z E
      "x:0" Z "()" Z "^" Z E
      "x:1" Z "{}" Z "^^" Z E
      "ā" Z "ā" Z E
      "ī" Z "ī" Z E
      "ū" Z "ū" Z E
      "ĕ" Z "ĕ" Z E
      "ŏ" Z "ŏ" Z E
      "ô" Z "ô" Z E
      "ṅ" Z "ṅ" Z E
      "ñ" Z "ñ" Z E
      "ṭ" Z "ṭ" Z E
      "ṭʰ" Z "ṭʰ" Z E
      "ḍ" Z "ḍ" Z E
      "ḍʰ" Z "ḍʰ" Z E
      "ṇ" Z "ṇ" Z E
      "ś" Z "ś" Z E
      "ṣ" Z "ṣ" Z E
      "ḷ" Z "ḷ" Z E
      "ġ" Z "ġ" Z E
      "ṛ" Z "ṛ" Z E
      "ṛʰ" Z "ṛʰ" Z E
      "ẏ" Z "ẏ" Z E
      "n̆ḍ" Z "n̆ḍ" Z E
      "ṃ" Z "ṃ" Z E
      "ḥ" Z "ḥ" Z E
      "oṃ" Z "oṃ" Z E
    E
  E
  "wx" Z LATIN
    VOWELS "a" Z "A" Z "i" Z "I" Z "u" Z "U" Z "q" Z "qV" Z "L" Z "LV" Z "eV" Z "e" Z "E" Z "oV" Z "o" Z "O" Z "aEV" Z "AEV" Z "aOV" Z E
    VOWELMARKS "" Z "A" Z "i" Z "I" Z "u" Z "U" Z "q" Z "qV" Z "L" Z "LV" Z "eV" Z "e" Z "E" Z "oV" Z "o" Z "O" Z "aEV" Z "AEV" Z "aOV" Z E
    CONSONANTS "k" Z "K" Z "g" Z "G" Z "f" Z "c" Z "C" Z "j" Z "J" Z "F" Z "t" Z "T" Z "d" Z "D" Z "N" Z "w" Z "W" Z "x" Z "X" Z "n" Z "p" Z "P" Z "b" Z "B" Z "m" Z "y" Z "r" Z "l" Z "v" Z "S" Z "R" Z "s" Z "h" Z "lY" Z "lYY" Z "rY" Z "nY" Z "kY" Z "KY" Z "GY" Z "zY" Z "dY" Z "DY" Z "PY" Z "yY" Z "nYYg" Z "nYYj" Z "nYYd" Z "nYYx" Z "mYYb" Z E
    OTHERDIACRITICS "z" Z "M" Z "H" Z "kYY" Z E
    SYMBOLS "0" Z "1" Z "2" Z "3" Z "4" Z "5" Z "6" Z "7" Z "8" Z "9" Z "oM" Z "." Z ".." Z E
    VEDICSYMBOLS "Z" Z "gͫ" Z "gͫ̄" Z E
    EQUIVALENTS
      "x:0" Z "()" Z "^" Z E
      "x:1" Z "{}" Z "^^" Z E
    E
  E
;

// clang-format on
