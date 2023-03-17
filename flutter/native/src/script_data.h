#pragma once

#define Z "\u0000"
#define E "\u0001"

#define INDIC "i\u0000"
#define TAMIL "t\u0000"
#define LATIN "l\u0000"

#define VOWELS "v\u0000"
#define VOWELDIACRITICS "V\u0000"
#define CONSONANTS "c\u0000"
#define COMMONDIACRITICS "C\u0000"
#define SYMBOLS "s\u0000"
#define ALTERNATES "a\u0000"
#define ALIASES "A\u0000"
#define LANGUAGES "l\u0000"

// clang-format off

const char scriptData[] =
  "assamese" Z INDIC
    VOWELS "অ" Z "আ" Z "ই" Z "ঈ" Z "উ" Z "ঊ" Z "ঋ" Z "ৠ" Z "ঌ" Z "ৡ" Z "এ᳴" Z "এ" Z "ঐ" Z "ও᳴" Z "ও" Z "ঔ" Z "এ᳴" Z "এˇ" Z "আ᳴" Z E
    VOWELDIACRITICS "্" Z "া" Z "ি" Z "ী" Z "ু" Z "ূ" Z "ৃ" Z "ৄ" Z "ৢ" Z "ৣ" Z "ে᳴" Z "ে" Z "ৈ" Z "ো᳴" Z "ো" Z "ৌ" Z "ে᳴" Z "েˇ" Z "া᳴" Z E
    CONSONANTS "ক" Z "খ" Z "গ" Z "ঘ" Z "ঙ" Z "চ" Z "ছ" Z "জ" Z "ঝ" Z "ঞ" Z "ট" Z "ঠ" Z "ড" Z "ঢ" Z "ণ" Z "ত" Z "থ" Z "দ" Z "ধ" Z "ন" Z "প" Z "ফ" Z "ব" Z "ভ" Z "ম" Z "য" Z "র" Z "ল" Z "ৱ" Z "শ" Z "ষ" Z "স" Z "হ" Z "ল়" Z "ষ়" Z "র়" Z "ন়" Z "ক়" Z "খ়" Z "গ়" Z "জ়" Z "ড়" Z "ঢ়" Z "ফ়" Z "য়" Z "ঁˆগ" Z "ঁˆজ" Z "ঁˆড" Z "ঁˆদ" Z "ঁˆব" Z E
    COMMONDIACRITICS "ঁ" Z "ং" Z "ঃ" Z "ঃʼ" Z E
    SYMBOLS "০" Z "১" Z "২" Z "৩" Z "৪" Z "৫" Z "৬" Z "৭" Z "৮" Z "৯" Z "ওঁ" Z "।" Z "॥" Z "ঽ" Z "ꣳ" Z "ꣴ" Z "​" Z "‌" Z "‍" Z E
  E
  "bengali" Z INDIC
    ALIASES "bangla" Z E
    VOWELS "অ" Z "আ" Z "ই" Z "ঈ" Z "উ" Z "ঊ" Z "ঋ" Z "ৠ" Z "ঌ" Z "ৡ" Z "এ᳴" Z "এ" Z "ঐ" Z "ও᳴" Z "ও" Z "ঔ" Z "এ᳴" Z "এˇ" Z "আ᳴" Z E
    VOWELDIACRITICS "্" Z "া" Z "ি" Z "ী" Z "ু" Z "ূ" Z "ৃ" Z "ৄ" Z "ৢ" Z "ৣ" Z "ে᳴" Z "ে" Z "ৈ" Z "ো᳴" Z "ো" Z "ৌ" Z "ে᳴" Z "েˇ" Z "া᳴" Z E
    CONSONANTS "ক" Z "খ" Z "গ" Z "ঘ" Z "ঙ" Z "চ" Z "ছ" Z "জ" Z "ঝ" Z "ঞ" Z "ট" Z "ঠ" Z "ড" Z "ঢ" Z "ণ" Z "ত" Z "থ" Z "দ" Z "ধ" Z "ন" Z "প" Z "ফ" Z "ব" Z "ভ" Z "ম" Z "য" Z "র" Z "ল" Z "ৱ" Z "শ" Z "ষ" Z "স" Z "হ" Z "ল়" Z "ষ়" Z "র়" Z "ন়" Z "ক়" Z "খ়" Z "গ়" Z "জ়" Z "ড়" Z "ঢ়" Z "ফ়" Z "য়" Z "ঁˆগ" Z "ঁˆজ" Z "ঁˆড" Z "ঁˆদ" Z "ঁˆব" Z E
    COMMONDIACRITICS "ঁ" Z "ং" Z "ঃ" Z "ঃʼ" Z E
    SYMBOLS "০" Z "১" Z "২" Z "৩" Z "৪" Z "৫" Z "৬" Z "৭" Z "৮" Z "৯" Z "ওঁ" Z "।" Z "॥" Z "ঽ" Z "ꣳ" Z "ꣴ" Z "​" Z "‌" Z "‍" Z E
  E
  "devanagari" Z INDIC
    ALIASES "samskrit" Z "sanskrit" Z E
    VOWELS "अ" Z "आ" Z "इ" Z "ई" Z "उ" Z "ऊ" Z "ऋ" Z "ॠ" Z "ऌ" Z "ॡ" Z "ऎ" Z "ए" Z "ऐ" Z "ऒ" Z "ओ" Z "औ" Z "ऍ" Z "एॕ" Z "ऑ" Z E
    VOWELDIACRITICS "्" Z "ा" Z "ि" Z "ी" Z "ु" Z "ू" Z "ृ" Z "ॄ" Z "ॢ" Z "ॣ" Z "ॆ" Z "े" Z "ै" Z "ॊ" Z "ो" Z "ौ" Z "ॅ" Z "ॕ" Z "ॉ" Z E
    CONSONANTS "क" Z "ख" Z "ग" Z "घ" Z "ङ" Z "च" Z "छ" Z "ज" Z "झ" Z "ञ" Z "ट" Z "ठ" Z "ड" Z "ढ" Z "ण" Z "त" Z "थ" Z "द" Z "ध" Z "न" Z "प" Z "फ" Z "ब" Z "भ" Z "म" Z "य" Z "र" Z "ल" Z "व" Z "श" Z "ष" Z "स" Z "ह" Z "ळ" Z "ऴ" Z "ऱ" Z "ऩ" Z "क़" Z "ख़" Z "ग़" Z "ज़" Z "ड़" Z "ढ़" Z "फ़" Z "य़" Z "ँˆग" Z "ँˆज" Z "ँˆड" Z "ँˆद" Z "ँˆब" Z E
    COMMONDIACRITICS "ँ" Z "ं" Z "ः" Z "ःʼ" Z E
    SYMBOLS "०" Z "१" Z "२" Z "३" Z "४" Z "५" Z "६" Z "७" Z "८" Z "९" Z "ॐ" Z "।" Z "॥" Z "ऽ" Z "ꣳ" Z "ꣴ" Z "​" Z "‌" Z "‍" Z E
    LANGUAGES
      "hindi" Z E
    E
    ALTERNATES
      "ॐ" Z "ओ३म्" Z E
      "क़" Z "क़" Z E
      "ख़" Z "ख़" Z E
      "ग़" Z "ग़" Z E
      "ज़" Z "ज़" Z E
      "ड़" Z "ड़" Z E
      "ढ़" Z "ढ़" Z E
      "फ़" Z "फ़" Z E
      "य़" Z "य़" Z E
      "्" Z "᳭" Z "്‍" Z "‍್" Z "්‍" Z "्‍" Z E
    E
  E
  "gujarati" Z INDIC
    VOWELS "અ" Z "આ" Z "ઇ" Z "ઈ" Z "ઉ" Z "ઊ" Z "ઋ" Z "ૠ" Z "ઌ" Z "ૡ" Z "એ᳴" Z "એ" Z "ઐ" Z "ઓ᳴" Z "ઓ" Z "ઔ" Z "ઍ" Z "ઍˇ" Z "ઑ" Z E
    VOWELDIACRITICS "્" Z "ા" Z "િ" Z "ી" Z "ુ" Z "ૂ" Z "ૃ" Z "ૄ" Z "ૢ" Z "ૣ" Z "ે᳴" Z "ે" Z "ૈ" Z "ો᳴" Z "ો" Z "ૌ" Z "ૅ" Z "ૅˇ" Z "ૉ" Z E
    CONSONANTS "ક" Z "ખ" Z "ગ" Z "ઘ" Z "ઙ" Z "ચ" Z "છ" Z "જ" Z "ઝ" Z "ઞ" Z "ટ" Z "ઠ" Z "ડ" Z "ઢ" Z "ણ" Z "ત" Z "થ" Z "દ" Z "ધ" Z "ન" Z "પ" Z "ફ" Z "બ" Z "ભ" Z "મ" Z "ય" Z "ર" Z "લ" Z "વ" Z "શ" Z "ષ" Z "સ" Z "હ" Z "ળ" Z "ળ઼" Z "ર઼" Z "ન઼" Z "ક઼" Z "ખ઼" Z "ગ઼" Z "જ઼" Z "ડ઼" Z "ઢ઼" Z "ફ઼" Z "ય઼" Z "ઁˆગ" Z "ઁˆજ" Z "ઁˆડ" Z "ઁˆદ" Z "ઁˆબ" Z E
    COMMONDIACRITICS "ઁ" Z "ં" Z "ઃ" Z "ઃʼ" Z E
    SYMBOLS "૦" Z "૧" Z "૨" Z "૩" Z "૪" Z "૫" Z "૬" Z "૭" Z "૮" Z "૯" Z "ૐ" Z "।" Z "॥" Z "ઽ" Z "ꣳ" Z "ꣴ" Z "​" Z "‌" Z "‍" Z E
  E
  "gurmukhi" Z INDIC
    ALIASES "punjabi" Z E
    VOWELS "ਅ" Z "ਆ" Z "ਇ" Z "ਈ" Z "ਉ" Z "ਊ" Z "ਰੁʼ" Z "ਰੂʼ" Z "ਲੁʼ" Z "ਲੂʼ" Z "ਏ᳴" Z "ਏ" Z "ਐ" Z "ਓ᳴" Z "ਓ" Z "ਔ" Z "ਏ᳴" Z "ਏˇ" Z "ਆ᳴" Z E
    VOWELDIACRITICS "੍" Z "ਾ" Z "ਿ" Z "ੀ" Z "ੁ" Z "ੂ" Z "੍ਰੁʼ" Z "੍ਰੂʼ" Z "੍ਲੁʼ" Z "੍ਲੂʼ" Z "ੇ᳴" Z "ੇ" Z "ੈ" Z "ੋ᳴" Z "ੋ" Z "ੌ" Z "ੇ᳴" Z "ੇˇ" Z "ਾ᳴" Z E
    CONSONANTS "ਕ" Z "ਖ" Z "ਗ" Z "ਘ" Z "ਙ" Z "ਚ" Z "ਛ" Z "ਜ" Z "ਝ" Z "ਞ" Z "ਟ" Z "ਠ" Z "ਡ" Z "ਢ" Z "ਣ" Z "ਤ" Z "ਥ" Z "ਦ" Z "ਧ" Z "ਨ" Z "ਪ" Z "ਫ" Z "ਬ" Z "ਭ" Z "ਮ" Z "ਯ" Z "ਰ" Z "ਲ" Z "ਵ" Z "ਸ਼" Z "ਸ਼਼" Z "ਸ" Z "ਹ" Z "ਲ਼" Z "ਲ਼਼" Z "ਰ਼" Z "ਨ਼" Z "ਕ਼" Z "ਖ਼" Z "ਗ਼" Z "ਜ਼" Z "ੜ" Z "ਢ਼" Z "ਫ਼" Z "ਯ਼" Z "ਁˆਗ" Z "ਁˆਜ" Z "ਁˆਡ" Z "ਁˆਦ" Z "ਁˆਬ" Z E
    COMMONDIACRITICS "ਁ" Z "ਂ" Z "ਃ" Z "ਃʼ" Z E
    SYMBOLS "੦" Z "੧" Z "੨" Z "੩" Z "੪" Z "੫" Z "੬" Z "੭" Z "੮" Z "੯" Z "ੴ" Z "।" Z "॥" Z "(ਅ)" Z "ꣳ" Z "ꣴ" Z "​" Z "‌" Z "‍" Z E
  E
  "iast" Z LATIN
    VOWELS "a" Z "ā" Z "i" Z "ī" Z "u" Z "ū" Z "ṛ" Z "ṝ" Z "ḷ" Z "ḹ" Z "ĕ" Z "e" Z "ai" Z "ŏ" Z "o" Z "au" Z "æ" Z "ǣ" Z "ô" Z E
    VOWELDIACRITICS "" Z "ā" Z "i" Z "ī" Z "u" Z "ū" Z "ṛ" Z "ṝ" Z "ḷ" Z "ḹ" Z "ĕ" Z "e" Z "ai" Z "ŏ" Z "o" Z "au" Z "æ" Z "ǣ" Z "ô" Z E
    CONSONANTS "k" Z "kh" Z "g" Z "gh" Z "ṅ" Z "c" Z "ch" Z "j" Z "jh" Z "ñ" Z "ṭ" Z "ṭh" Z "ḍ" Z "ḍh" Z "ṇ" Z "t" Z "th" Z "d" Z "dh" Z "n" Z "p" Z "ph" Z "b" Z "bh" Z "m" Z "y" Z "r" Z "l" Z "v" Z "ś" Z "ṣ" Z "s" Z "h" Z "l̤" Z "ḻ" Z "ṟ" Z "ṉ" Z "q" Z "k͟h" Z "ġ" Z "z" Z "r̤" Z "r̤h" Z "f" Z "ẏ" Z "n̆g" Z "n̆j" Z "n̆ḍ" Z "n̆d" Z "m̆b" Z E
    COMMONDIACRITICS "m̐" Z "ṃ" Z "ḥ" Z "ḵ" Z E
    SYMBOLS "0" Z "1" Z "2" Z "3" Z "4" Z "5" Z "6" Z "7" Z "8" Z "9" Z "oṃ" Z "." Z ".." Z "'" Z "gͫ" Z "gͫ̄" Z "​" Z "‌" Z "‍" Z E
    ALTERNATES
      "s:17" Z "{}" Z "^^" Z E
      "s:18" Z "()" Z "^" Z E
    E
  E
  "ipa" Z LATIN
    VOWELS "ə" Z "ɑː" Z "ɪ" Z "iː" Z "u" Z "uː" Z "ɾɨ" Z "ɾɨː" Z "lɨ" Z "lɨː" Z "e" Z "eː" Z "əɪ" Z "o" Z "oː" Z "əu" Z "æ" Z "æː" Z "ɔ" Z E
    VOWELDIACRITICS "" Z "ɑː" Z "ɪ" Z "iː" Z "u" Z "uː" Z "ɾɨ" Z "ɾɨː" Z "lɨ" Z "lɨː" Z "e" Z "eː" Z "əɪ" Z "o" Z "oː" Z "əu" Z "æ" Z "æː" Z "ɔ" Z E
    CONSONANTS "k" Z "kʰ" Z "g" Z "gʰ" Z "ŋ" Z "t͡ʃ" Z "t͡ʃʰ" Z "d͡ʒ" Z "d͡ʒʰ" Z "ɲ" Z "ʈ" Z "ʈʰ" Z "ɖ" Z "ɖʰ" Z "ɳ" Z "t̪" Z "t̪ʰ" Z "d̪" Z "d̪ʰ" Z "n̪" Z "p" Z "pʰ" Z "b" Z "bʰ" Z "m" Z "j" Z "ɾ" Z "l" Z "ʋ" Z "ʃ" Z "ʂ" Z "s̪" Z "ɦ" Z "ɭ" Z "ɻ" Z "r" Z "n" Z "q" Z "x" Z "ɣ" Z "z" Z "ɽ" Z "ɽʰ" Z "f" Z "ẏ" Z "ᵑɡ" Z "ᶮd͡ʒ" Z "ᶯɖ" Z "ⁿd" Z "ᵐb" Z E
    COMMONDIACRITICS "̃ʽ" Z "̃" Z "h" Z "xʽ" Z E
    SYMBOLS "0" Z "1" Z "2" Z "3" Z "4" Z "5" Z "6" Z "7" Z "8" Z "9" Z "oːm" Z "." Z ".." Z "ə̸" Z "gͫ" Z "gͫ̄" Z "​" Z "‌" Z "‍" Z E
    ALTERNATES
      "s:17" Z "{}" Z "^^" Z E
      "s:18" Z "()" Z "^" Z E
    E
  E
  "iso" Z LATIN
    VOWELS "a" Z "ā" Z "i" Z "ī" Z "u" Z "ū" Z "r̥" Z "r̥̄" Z "l̥" Z "l̥̄" Z "e" Z "ē" Z "ai" Z "o" Z "ō" Z "au" Z "æ" Z "ǣ" Z "ô" Z E
    VOWELDIACRITICS "" Z "ā" Z "i" Z "ī" Z "u" Z "ū" Z "r̥" Z "r̥̄" Z "l̥" Z "l̥̄" Z "e" Z "ē" Z "ai" Z "o" Z "ō" Z "au" Z "æ" Z "ǣ" Z "ô" Z E
    CONSONANTS "k" Z "kh" Z "g" Z "gh" Z "ṅ" Z "c" Z "ch" Z "j" Z "jh" Z "ñ" Z "ṭ" Z "ṭh" Z "ḍ" Z "ḍh" Z "ṇ" Z "t" Z "th" Z "d" Z "dh" Z "n" Z "p" Z "ph" Z "b" Z "bh" Z "m" Z "y" Z "r" Z "l" Z "v" Z "ś" Z "ṣ" Z "s" Z "h" Z "ḷ" Z "ḻ" Z "ṟ" Z "ṉ" Z "q" Z "k͟h" Z "ġ" Z "z" Z "ṛ" Z "ṛh" Z "f" Z "ẏ" Z "n̆g" Z "n̆j" Z "n̆ḍ" Z "n̆d" Z "m̆b" Z E
    COMMONDIACRITICS "m̐" Z "ṁ" Z "ḥ" Z "ḵ" Z E
    SYMBOLS "0" Z "1" Z "2" Z "3" Z "4" Z "5" Z "6" Z "7" Z "8" Z "9" Z "ōṁ" Z "." Z ".." Z "’" Z "gͫ" Z "gͫ̄" Z "​" Z "‌" Z "‍" Z E
    ALTERNATES
      "s:17" Z "{}" Z "^^" Z E
      "s:18" Z "()" Z "^" Z E
    E
  E
  "itrans" Z LATIN
    VOWELS "a" Z "A" Z "i" Z "I" Z "u" Z "U" Z "R^i" Z "R^I" Z "L^i" Z "L^I" Z "^e" Z "e" Z "ai" Z "^o" Z "o" Z "au" Z "e.c" Z "a.C" Z "A.c" Z E
    VOWELDIACRITICS "" Z "A" Z "i" Z "I" Z "u" Z "U" Z "R^i" Z "R^I" Z "L^i" Z "L^I" Z "^e" Z "e" Z "ai" Z "^o" Z "o" Z "au" Z "e.c" Z "a.C" Z "A.c" Z E
    CONSONANTS "k" Z "kh" Z "g" Z "gh" Z "~N" Z "ch" Z "Ch" Z "j" Z "jh" Z "~n" Z "T" Z "Th" Z "D" Z "Dh" Z "N" Z "t" Z "th" Z "d" Z "dh" Z "n" Z "p" Z "ph" Z "b" Z "bh" Z "m" Z "y" Z "r" Z "l" Z "v" Z "sh" Z "Sh" Z "s" Z "h" Z "L" Z "zh" Z "R" Z "^n" Z "q" Z "K" Z "G" Z "z" Z ".D" Z ".Dh" Z "f" Z "Y" Z "n*g" Z "n*j" Z "n*D" Z "n*d" Z "m*b" Z E
    COMMONDIACRITICS ".N" Z "M" Z "H" Z "K^" Z E
    SYMBOLS "0" Z "1" Z "2" Z "3" Z "4" Z "5" Z "6" Z "7" Z "8" Z "9" Z "oM" Z "." Z ".." Z ".a" Z "gͫ" Z "gͫ̄" Z "" Z "" Z "" Z E
    ALTERNATES
      "s:17" Z "{}" Z "^^" Z E
      "s:18" Z "()" Z "^" Z E
    E
  E
  "kannada" Z INDIC
    VOWELS "ಅ" Z "ಆ" Z "ಇ" Z "ಈ" Z "ಉ" Z "ಊ" Z "ಋ" Z "ೠ" Z "ಌ" Z "ೡ" Z "ಎ" Z "ಏ" Z "ಐ" Z "ಒ" Z "ಓ" Z "ಔ" Z "ಎ᳴" Z "ಏˇ" Z "ಆ᳴" Z E
    VOWELDIACRITICS "್" Z "ಾ" Z "ಿ" Z "ೀ" Z "ು" Z "ೂ" Z "ೃ" Z "ೄ" Z "ೢ" Z "ೣ" Z "ೆ" Z "ೇ" Z "ೈ" Z "ೊ" Z "ೋ" Z "ೌ" Z "ೆ᳴" Z "ೇˇ" Z "ಾ᳴" Z E
    CONSONANTS "ಕ" Z "ಖ" Z "ಗ" Z "ಘ" Z "ಙ" Z "ಚ" Z "ಛ" Z "ಜ" Z "ಝ" Z "ಞ" Z "ಟ" Z "ಠ" Z "ಡ" Z "ಢ" Z "ಣ" Z "ತ" Z "ಥ" Z "ದ" Z "ಧ" Z "ನ" Z "ಪ" Z "ಫ" Z "ಬ" Z "ಭ" Z "ಮ" Z "ಯ" Z "ರ" Z "ಲ" Z "ವ" Z "ಶ" Z "ಷ" Z "ಸ" Z "ಹ" Z "ಳ" Z "ೞ" Z "ಱ" Z "ನ಼" Z "ಕ಼" Z "ಖ಼" Z "ಗ಼" Z "ಜ಼" Z "ಡ಼" Z "ಢ಼" Z "ಫ಼" Z "ಯ಼" Z "ಂˆಗ" Z "ಂˆಜ" Z "ಂˆಡ" Z "ಂˆದ" Z "ಂˆಬ" Z E
    COMMONDIACRITICS "ಁ" Z "ಂ" Z "ಃ" Z "ಃʼ" Z E
    SYMBOLS "೦" Z "೧" Z "೨" Z "೩" Z "೪" Z "೫" Z "೬" Z "೭" Z "೮" Z "೯" Z "ಓಂ" Z "।" Z "॥" Z "ಽ" Z "ꣳ" Z "ꣴ" Z "​" Z "‌" Z "‍" Z E
  E
  "malayalam" Z INDIC
    VOWELS "അ" Z "ആ" Z "ഇ" Z "ഈ" Z "ഉ" Z "ഊ" Z "ഋ" Z "ൠ" Z "ഌ" Z "ൡ" Z "എ" Z "ഏ" Z "ഐ" Z "ഒ" Z "ഓ" Z "ഔ" Z "എ᳴" Z "ഏˇ" Z "ആ᳴" Z E
    VOWELDIACRITICS "്" Z "ാ" Z "ി" Z "ീ" Z "ു" Z "ൂ" Z "ൃ" Z "ൄ" Z "ൢ" Z "ൣ" Z "െ" Z "േ" Z "ൈ" Z "ൊ" Z "ോ" Z "ൗ" Z "െ᳴" Z "േˇ" Z "ാ᳴" Z E
    CONSONANTS "ക" Z "ഖ" Z "ഗ" Z "ഘ" Z "ങ" Z "ച" Z "ഛ" Z "ജ" Z "ഝ" Z "ഞ" Z "ട" Z "ഠ" Z "ഡ" Z "ഢ" Z "ണ" Z "ത" Z "ഥ" Z "ദ" Z "ധ" Z "ന" Z "പ" Z "ഫ" Z "ബ" Z "ഭ" Z "മ" Z "യ" Z "ര" Z "ല" Z "വ" Z "ശ" Z "ഷ" Z "സ" Z "ഹ" Z "ള" Z "ഴ" Z "റ" Z "ഩ" Z "ക·" Z "ഖ·" Z "ഗ·" Z "ജ·" Z "ഡ·" Z "ഢ·" Z "ഫ·" Z "യ·" Z "ംˆഗ" Z "ംˆജ" Z "ംˆഡ" Z "ംˆദ" Z "ംˆബ" Z E
    COMMONDIACRITICS "ഁ" Z "ം" Z "ഃ" Z "ഃʼ" Z E
    SYMBOLS "൦" Z "൧" Z "൨" Z "൩" Z "൪" Z "൫" Z "൬" Z "൭" Z "൮" Z "൯" Z "ഓം" Z "।" Z "॥" Z "ഽ" Z "ꣳ" Z "ꣴ" Z "​" Z "‌" Z "‍" Z E
  E
  "oriya" Z INDIC
    VOWELS "ଅ" Z "ଆ" Z "ଇ" Z "ଈ" Z "ଉ" Z "ଊ" Z "ଋ" Z "ୠ" Z "ଌ" Z "ୡ" Z "ଏ᳴" Z "ଏ" Z "ଐ" Z "ଓ᳴" Z "ଓ" Z "ଔ" Z "ଏ᳴" Z "ଏˇ" Z "ଆ᳴" Z E
    VOWELDIACRITICS "୍" Z "ା" Z "ି" Z "ୀ" Z "ୁ" Z "ୂ" Z "ୃ" Z "ୄ" Z "ୢ" Z "ୣ" Z "େ᳴" Z "େ" Z "ୈ" Z "ୋ᳴" Z "ୋ" Z "ୌ" Z "େ᳴" Z "େˇ" Z "ା᳴" Z E
    CONSONANTS "କ" Z "ଖ" Z "ଗ" Z "ଘ" Z "ଙ" Z "ଚ" Z "ଛ" Z "ଜ" Z "ଝ" Z "ଞ" Z "ଟ" Z "ଠ" Z "ଡ" Z "ଢ" Z "ଣ" Z "ତ" Z "ଥ" Z "ଦ" Z "ଧ" Z "ନ" Z "ପ" Z "ଫ" Z "ବ" Z "ଭ" Z "ମ" Z "ଯ" Z "ର" Z "ଲ" Z "ୱ" Z "ଶ" Z "ଷ" Z "ସ" Z "ହ" Z "ଳ" Z "ଳ଼" Z "ର଼" Z "ନ଼" Z "କ଼" Z "ଖ଼" Z "ଗ଼" Z "ଜ଼" Z "ଡ଼" Z "ଢ଼" Z "ଫ଼" Z "ୟ" Z "ଁˆଗ" Z "ଁˆଜ" Z "ଁˆଡ" Z "ଁˆଦ" Z "ଁˆବ" Z E
    COMMONDIACRITICS "ଁ" Z "ଂ" Z "ଃ" Z "ଃʼ" Z E
    SYMBOLS "୦" Z "୧" Z "୨" Z "୩" Z "୪" Z "୫" Z "୬" Z "୭" Z "୮" Z "୯" Z "ଓଁ" Z "।" Z "॥" Z "ଽ" Z "ꣳ" Z "ꣴ" Z "​" Z "‌" Z "‍" Z E
  E
  "readablelatin" Z LATIN
    VOWELS "a" Z "aa" Z "i" Z "ee" Z "u" Z "oo" Z "ru̥" Z "rru̥" Z "lu̥" Z "llu̥" Z "e" Z "E" Z "ai" Z "o" Z "O" Z "au" Z "a" Z "e" Z "o" Z E
    VOWELDIACRITICS "" Z "aa" Z "i" Z "ee" Z "u" Z "oo" Z "ru̥" Z "rru̥" Z "lu̥" Z "llu̥" Z "e" Z "E" Z "ai" Z "o" Z "O" Z "au" Z "a" Z "e" Z "o" Z E
    CONSONANTS "k" Z "kh" Z "g" Z "gh" Z "n" Z "ch" Z "chh" Z "j" Z "jh" Z "n" Z "T" Z "Th" Z "D" Z "Dh" Z "N" Z "t" Z "th" Z "d" Z "dh" Z "n" Z "p" Z "ph" Z "b" Z "bh" Z "m" Z "y" Z "r" Z "l" Z "v" Z "sh" Z "sh" Z "s" Z "h" Z "L" Z "zh" Z "R" Z "n" Z "q" Z "qh" Z "gh" Z "z" Z "D" Z "Dh" Z "f" Z "y" Z "ng" Z "nj" Z "nD" Z "nd" Z "mb" Z E
    COMMONDIACRITICS "n̐" Z "" Z "h" Z "q" Z E
    SYMBOLS "0" Z "1" Z "2" Z "3" Z "4" Z "5" Z "6" Z "7" Z "8" Z "9" Z "om" Z "!" Z "!!" Z "~" Z "gum" Z "ggum" Z "​" Z "‌" Z "‍" Z E
  E
  "sinhala" Z INDIC
    VOWELS "අ" Z "ආ" Z "ඉ" Z "ඊ" Z "උ" Z "ඌ" Z "ඍ" Z "ඎ" Z "ඏ" Z "ඐ" Z "එ" Z "ඒ" Z "ඓ" Z "ඔ" Z "ඕ" Z "ඖ" Z "ඇ" Z "ඈ" Z "ආ᳴" Z E
    VOWELDIACRITICS "්" Z "ා" Z "ි" Z "ී" Z "ු" Z "ූ" Z "ෘ" Z "ෲ" Z "ෟ" Z "ෳ" Z "ෙ" Z "ේ" Z "ෛ" Z "ො" Z "ෝ" Z "ෞ" Z "ැ" Z "ෑ" Z "ා᳴" Z E
    CONSONANTS "ක" Z "ඛ" Z "ග" Z "ඝ" Z "ඞ" Z "ච" Z "ඡ" Z "ජ" Z "ඣ" Z "ඤ" Z "ට" Z "ඨ" Z "ඩ" Z "ඪ" Z "ණ" Z "ත" Z "ථ" Z "ද" Z "ධ" Z "න" Z "ප" Z "ඵ" Z "බ" Z "භ" Z "ම" Z "ය" Z "ර" Z "ල" Z "ව" Z "ශ" Z "ෂ" Z "ස" Z "හ" Z "ළ" Z "ළ·" Z "ර·" Z "න·" Z "ක·" Z "ඛ·" Z "ග·" Z "ජ·" Z "ඩ·" Z "ඪ·" Z "ෆ" Z "ය·" Z "ඟ" Z "ඦ" Z "ඬ" Z "ඳ" Z "ඹ" Z E
    COMMONDIACRITICS "ංʼ" Z "ං" Z "ඃ" Z "ඃʼ" Z E
    SYMBOLS "෦" Z "෧" Z "෨" Z "෩" Z "෪" Z "෫" Z "෬" Z "෭" Z "෮" Z "෯" Z "ඕං" Z "।" Z "॥" Z "(අ)" Z "ꣳ" Z "ꣴ" Z "​" Z "‌" Z "‍" Z E
  E
  "tamil" Z TAMIL
    VOWELS "அ" Z "ஆ" Z "இ" Z "ஈ" Z "உ" Z "ஊ" Z "ருʼ" Z "ரூʼ" Z "லுʼ" Z "லூʼ" Z "எ" Z "ஏ" Z "ஐ" Z "ஒ" Z "ஓ" Z "ஔ" Z "எ᳴" Z "ஏ᳴" Z "ஆ᳴" Z E
    VOWELDIACRITICS "்" Z "ா" Z "ி" Z "ீ" Z "ு" Z "ூ" Z "்ருʼ" Z "்ரூʼ" Z "்லுʼ" Z "்லூʼ" Z "ெ" Z "ே" Z "ை" Z "ொ" Z "ோ" Z "ௌ" Z "ெ᳴" Z "ே᳴" Z "ா᳴" Z E
    CONSONANTS "க" Z "க²" Z "க³" Z "க⁴" Z "ங" Z "ச" Z "ச²" Z "ஜ" Z "ஜ²" Z "ஞ" Z "ட" Z "ட²" Z "ட³" Z "ட⁴" Z "ண" Z "த" Z "த²" Z "த³" Z "த⁴" Z "ந" Z "ப" Z "ப²" Z "ப³" Z "ப⁴" Z "ம" Z "ய" Z "ர" Z "ல" Z "வ" Z "ஶ" Z "ஷ" Z "ஸ" Z "ஹ" Z "ள" Z "ழ" Z "ற" Z "ன" Z "ஃக" Z "ஃக²" Z "ஃக³" Z "ஃஜ" Z "ஃட²" Z "ஃட³" Z "ஃப" Z "ஃய" Z "ங்ˆக³" Z "ஞ்ˆஜ³" Z "ண்ˆட³" Z "ந்ˆத³" Z "ம்ˆப³" Z E
    COMMONDIACRITICS "னஂ" Z "மஂ" Z "꞉" Z "ஃ" Z E
    SYMBOLS "௦" Z "௧" Z "௨" Z "௩" Z "௪" Z "௫" Z "௬" Z "௭" Z "௮" Z "௯" Z "ௐ" Z "।" Z "॥" Z "(அ)" Z "ꣳ" Z "ꣴ" Z "​" Z "‌" Z "‍" Z E
    ALTERNATES
      "ொ" Z "ொ" Z E
      "ோ" Z "ோ" Z E
      "ௌ" Z "ௌ" Z E
      "னஂ" Z "ம்ˮ" Z E
      "மஂ" Z "ம்ʼ" Z E
    E
  E
  "tamil-extended" Z INDIC
    VOWELS "അ" Z "ആ" Z "ഇ" Z "ഈ" Z "ഉ" Z "ഊ" Z "ഋ" Z "ൠ" Z "ഌ" Z "ൡ" Z "എ" Z "ഏ" Z "ഐ" Z "ഒ" Z "ഓ" Z "ഔ" Z "എ᳴" Z "ഏ᳴" Z "ആ᳴" Z E
    VOWELDIACRITICS "്‌" Z "ാ" Z "ി" Z "ീ" Z "ു" Z "ൂ" Z "ൃ" Z "ൄ" Z "ൢ" Z "ൣ" Z "െ" Z "േ" Z "ൈ" Z "ൊ" Z "ോ" Z "ൗ" Z "െ᳴" Z "േ᳴" Z "ാ᳴" Z E
    CONSONANTS "ക" Z "ഖ" Z "ഗ" Z "ഘ" Z "ങ" Z "ച" Z "ഛ" Z "ജ" Z "ഝ" Z "ഞ" Z "ട" Z "ഠ" Z "ഡ" Z "ഢ" Z "ണ" Z "ത" Z "ഥ" Z "ദ" Z "ധ" Z "ന" Z "പ" Z "ഫ" Z "ബ" Z "ഭ" Z "മ" Z "യ" Z "ര" Z "ല" Z "വ" Z "ശ" Z "ഷ" Z "സ" Z "ഹ" Z "ള" Z "ഴ" Z "റ" Z "ഩ" Z "ക·" Z "ഖ·" Z "ഗ·" Z "ജ·" Z "ഡ·" Z "ഢ·" Z "ഫ·" Z "യ·" Z "ംˆഗ" Z "ംˆജ" Z "ംˆഡ" Z "ംˆദ" Z "ംˆബ" Z E
    COMMONDIACRITICS "ഁ" Z "ം" Z "ഃ" Z "ஃ" Z E
    SYMBOLS "௦" Z "௧" Z "௨" Z "௩" Z "௪" Z "௫" Z "௬" Z "௭" Z "௮" Z "௯" Z "ௐ" Z "ഽ" Z "।" Z "॥" Z "ꣳ" Z "ꣴ" Z "​" Z "‌" Z "‍" Z E
  E
  "telugu" Z INDIC
    VOWELS "అ" Z "ఆ" Z "ఇ" Z "ఈ" Z "ఉ" Z "ఊ" Z "ఋ" Z "ౠ" Z "ఌ" Z "ౡ" Z "ఎ" Z "ఏ" Z "ఐ" Z "ఒ" Z "ఓ" Z "ఔ" Z "ఎ᳴​" Z "ఏ᳴" Z "ఒ᳴​" Z E
    VOWELDIACRITICS "్" Z "ా" Z "ి" Z "ీ" Z "ు" Z "ూ" Z "ృ" Z "ౄ" Z "ౢ" Z "ౣ" Z "ె" Z "ే" Z "ై" Z "ొ" Z "ో" Z "ౌ" Z "ె॒​" Z "ే॒​" Z "ొ॒​" Z E
    CONSONANTS "క" Z "ఖ" Z "గ" Z "ఘ" Z "ఙ" Z "చ" Z "ఛ" Z "జ" Z "ఝ" Z "ఞ" Z "ట" Z "ఠ" Z "డ" Z "ఢ" Z "ణ" Z "త" Z "థ" Z "ద" Z "ధ" Z "న" Z "ప" Z "ఫ" Z "బ" Z "భ" Z "మ" Z "య" Z "ర" Z "ల" Z "వ" Z "శ" Z "ష" Z "స" Z "హ" Z "ళ" Z "ఴ" Z "ఱ" Z "న·" Z "క·" Z "ఖ·" Z "గ·" Z "జ·" Z "డ·" Z "ఢ·" Z "ఫ·" Z "య·" Z "ఁˆగ" Z "ఁˆజ" Z "ఁˆడ" Z "ఁˆద" Z "ఁˆబ" Z E
    COMMONDIACRITICS "ఀ" Z "ం" Z "ః" Z "ఃʼ" Z E
    SYMBOLS "౦" Z "౧" Z "౨" Z "౩" Z "౪" Z "౫" Z "౬" Z "౭" Z "౮" Z "౯" Z "ఓం" Z "।" Z "॥" Z "ఽ" Z "ꣳ" Z "ꣴ" Z "​" Z "‌" Z "‍" Z E
  E
;

// clang-format on
