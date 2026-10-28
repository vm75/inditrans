// Keyboard tables and script catalog for the buildless demo.
// Keyboard symbols come from Inditrans script/reader tables; unavailable scripts are disabled.
const KEYBOARD_MAPS = {
  assamese: {
    vowels: ["অ", "আ", "ই", "ঈ", "উ", "ঊ", "ঋ", "ৠ", "ঌ", "ৡ", "এ", "এ", "ঐ", "ও", "ও", "ঔ", "অ্যা", "অ্যা", "অ", "অ্যা", "উ", "অ্যা"],
    consonants: ["ক", "খ", "গ", "ঘ", "ঙ", "চ", "ছ", "জ", "ঝ", "ঞ", "ট", "ঠ", "ড", "ঢ", "ণ", "ত", "থ", "দ", "ধ", "ন", "প", "ফ", "ব", "ভ", "ম", "য", "ৰ", "ল", "ৱ", "শ", "ষ", "স", "হ", "ল়", "ষ়", "ৰ়", "ন়", "ক়", "খ়", "গ়", "জ়", "ড়", "ঢ়", "ফ়", "য়", "ঁˆগ", "ঁˆজ", "ঁˆড", "ঁˆদ", "ঁˆব", "চ", "ছ", "ত", "ঝ়", "ৱ়", "স", "স", "হ", "ত", "হ", "জ়", "জ়", "জ়", "জ"],
    vowelMarks: ["্", "া", "ি", "ী", "ু", "ূ", "ৃ", "ৄ", "ৢ", "ৣ", "ে", "ে", "ৈ", "ো", "ো", "ৌ", "্যা", "্যা", "", "্যা", "ু", "্যা"],
    otherDiacritics: ["ঁ", "ং", "ঃ", "ঃ", "ঃ", "ঃ", "ঁ", "ং"],
    symbols: ["০", "১", "২", "৩", "৪", "৫", "৬", "৭", "৮", "৯", "ওঁ", "।", "॥"],
    vedicSymbols: ["ঽ", "ꣳ", "ꣴ"],
  },
  burmese: {
    vowels: ["အ", "အာ", "ဣ", "ဤ", "ဥ", "ဦ", "ၒ", "ၓ", "ၔ", "ၕ", "ဧ", "ဧ", "အဲ", "ဩ", "ဩ", "ဪ", "အဲ", "အဲ", "ဩ", "ဧ", "ဥ", "အဲ"],
    consonants: ["က", "ခ", "ဂ", "ဃ", "င", "စ", "ဆ", "ဇ", "ဈ", "ဉ", "ဋ", "ဌ", "ဍ", "ဎ", "ဏ", "တ", "ထ", "ဒ", "ဓ", "န", "ပ", "ဖ", "ဗ", "ဘ", "မ", "ယ", "ရ", "လ", "ဝ", "ၐ", "ၑ", "သ", "ဟ", "ဠ", "ဠ", "ရ", "န", "က", "ခ", "ဂ", "ဇ", "ဍ", "ဎ", "ဖ", "ယ", "ံဂ", "ံဇ", "ံဍ", "ံဒ", "ံဗ", "စ", "ဆ", "တ", "ဇ", "ဝ", "သ", "သ", "ဟ", "တ", "ဟ", "ဇ", "ဇ", "ဇ", "ဇ"],
    vowelMarks: ["်", "ာ", "ိ", "ီ", "ု", "ူ", "ၖ", "ၗ", "ၘ", "ၙ", "ေ", "ေ", "ဲ", "ော", "ော", "ော်", "ဲ", "ဲ", "ော", "ေ", "ု", "ဲ"],
    otherDiacritics: ["ံ", "ံ", "း", "း", "း", "း", "ံ", "ံ"],
    symbols: ["၀", "၁", "၂", "၃", "၄", "၅", "၆", "၇", "၈", "၉", "ဥုံ", "၊", "။"],
    vedicSymbols: ["'", "ꣳ", "ꣴ"],
  },
  khmer: {
    vowels: ["អ", "អា", "ឥ", "ឦ", "ឧ", "ឩ", "ឫ", "ឬ", "ឭ", "ឮ", "ឯ", "ឯ", "ឰ", "ឱ", "ឱ", "ឳ", "អែ", "អែ", "ឱ", "ឯ", "ឧ", "អែ"],
    consonants: ["ក", "ខ", "គ", "ឃ", "ង", "ច", "ឆ", "ជ", "ឈ", "ញ", "ដ", "ឋ", "ឌ", "ឍ", "ណ", "ត", "ថ", "ទ", "ធ", "ន", "ប", "ផ", "ព", "ភ", "ម", "យ", "រ", "ល", "វ", "ឝ", "ឞ", "ស", "ហ", "ឡ", "ឡ", "រ", "ន", "ក", "ខ", "គ", "ជ", "ឌ", "ឍ", "ផ", "យ", "ំគ", "ំជ", "ំឌ", "ំទ", "ំព", "ច", "ឆ", "ត", "ជ", "វ", "ស", "ស", "ហ", "ត", "ហ", "ជ", "ជ", "ជ", "ជ"],
    vowelMarks: ["៑", "ា", "ិ", "ី", "ុ", "ូ", "្ឫ", "្ឬ", "្ឭ", "្ឮ", "េ", "េ", "ៃ", "ោ", "ោ", "ៅ", "ែ", "ែ", "ោ", "េ", "ុ", "ែ"],
    otherDiacritics: ["ំ", "ំ", "ះ", "ះ", "ះ", "ះ", "ំ", "ំ"],
    symbols: ["០", "១", "២", "៣", "៤", "៥", "៦", "៧", "៨", "៩", "ឱំ", "។", "៕"],
    vedicSymbols: ["ៜ", "ꣳ", "ꣴ"],
  },
  ipa: {
    vowels: ["ə", "ɑː", "ɪ", "iː", "u", "uː", "ɾɨ", "ɾɨː", "lɨ", "lɨː", "e", "eː", "əɪ", "o", "oː", "əu", "æ", "æː", "ɔ", "ɛ", "ɯ", "ɛː"],
    consonants: ["k", "kʰ", "g", "gʰ", "ŋ", "t͡ʃ", "t͡ʃʰ", "d͡ʒ", "d͡ʒʰ", "ɲ", "ʈ", "ʈʰ", "ɖ", "ɖʰ", "ɳ", "t̪", "t̪ʰ", "d̪", "d̪ʰ", "n̪", "p", "pʰ", "b", "bʰ", "m", "j", "ɾ", "l", "ʋ", "ʃ", "ʂ", "s̪", "ɦ", "ɭ", "ɻ", "r", "n", "q", "x", "ɣ", "z", "ɽ", "ɽʰ", "f", "ẏ", "ᵑɡ", "ᶮd͡ʒ", "ᶯɖ", "ⁿd", "ᵐb", "t͡s", "t͡sʰ", "t", "ʒ", "w", "s", "s", "h", "t̪", "ʔ", "z", "z", "z", "d͡z"],
    vowelMarks: ["", "ɑː", "ɪ", "iː", "u", "uː", "ɾɨ", "ɾɨː", "lɨ", "lɨː", "e", "eː", "əɪ", "o", "oː", "əu", "æ", "æː", "ɔ", "ɛ", "ɯ", "ɛː"],
    otherDiacritics: ["̃ʽ", "̃", "h", "xʽ", "h", "h", "̃ʽ", "̃"],
    symbols: ["0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "oːm", ".", ".."],
    vedicSymbols: ["ə̸", "gͫ", "gͫ̄"],
  },
  grantha: {
    vowels: ["𑌅", "𑌆", "𑌇", "𑌈", "𑌉", "𑌊", "𑌋", "𑍠", "𑌌", "𑍡", "𑌏", "𑌏", "𑌐", "𑌓", "𑌓", "𑌔", "𑌏", "𑌏", "𑌓", "𑌏", "𑌉", "𑌏"],
    consonants: ["𑌕", "𑌖", "𑌗", "𑌘", "𑌙", "𑌚", "𑌛", "𑌜", "𑌝", "𑌞", "𑌟", "𑌠", "𑌡", "𑌢", "𑌣", "𑌤", "𑌥", "𑌦", "𑌧", "𑌨", "𑌪", "𑌫", "𑌬", "𑌭", "𑌮", "𑌯", "𑌰", "𑌲", "𑌵", "𑌶", "𑌷", "𑌸", "𑌹", "𑌳", "𑌳", "𑌰", "𑌨", "𑌕", "𑌖", "𑌗", "𑌜", "𑌡", "𑌢", "𑌫", "𑌯", "𑌂𑌗", "𑌂𑌜", "𑌂𑌡", "𑌂𑌦", "𑌂𑌬", "𑌚", "𑌛", "𑌤", "𑌜", "𑌵", "𑌸", "𑌸", "𑌹", "𑌤", "𑌹", "𑌜", "𑌜", "𑌜", "𑌜"],
    vowelMarks: ["𑍍", "𑌾", "𑌿", "𑍀", "𑍁", "𑍂", "𑍃", "𑍄", "𑍢", "𑍣", "𑍇", "𑍇", "𑍈", "𑍋", "𑍋", "𑍌", "𑍇", "𑍇", "𑍋", "𑍇", "𑍁", "𑍇"],
    otherDiacritics: ["𑌁", "𑌂", "𑌃", "𑌃", "ᳵ", "ᳶ", "𑌁", "𑌂"],
    symbols: ["௦", "௧", "௨", "௩", "௪", "௫", "௬", "௭", "௮", "௯", "𑍐", "।", "॥"],
    vedicSymbols: ["𑌽", "𑍞", "𑍟"],
  },
  thai: {
    vowels: ["อ", "อา", "อิ", "อี", "อุ", "อู", "ฤ", "ฤๅ", "ฦ", "ฦๅ", "เอะ", "เอ", "ไอ", "โอะ", "โอ", "เอา", "แอะ", "แอ", "เอาะ", "แอะ", "อุ", "แอ"],
    consonants: ["ก", "ข", "ค", "ฆ", "ง", "จ", "ฉ", "ช", "ฌ", "ญ", "ฏ", "ฐ", "ฑ", "ฒ", "ณ", "ต", "ถ", "ท", "ธ", "น", "ป", "ผ", "พ", "ภ", "ม", "ย", "ร", "ล", "ว", "ศ", "ษ", "ส", "ห", "ฬ", "ฬ", "ร", "น", "ก", "ฃ", "ฅ", "ซ", "ร", "รฺห", "ฟ", "ย", "ํค", "ํช", "ํฑ", "ํท", "ํพ", "จ", "ฉ", "ต", "ช", "ว", "ส", "ส", "ห", "ต", "ห", "ซ", "ซ", "ซ", "ช"],
    vowelMarks: ["ฺ", "า", "ิ", "ี", "ุ", "ู", "ฺฤ", "ฺฤๅ", "ฺฦ", "ฺฦๅ", "เะ", "เ", "ไ", "โะ", "โ", "เา", "แะ", "แ", "เาะ", "แะ", "ุ", "แ"],
    otherDiacritics: ["ํ", "ํ", "ะ", "ะ", "ะ", "ะ", "ํ", "ํ"],
    symbols: ["๐", "๑", "๒", "๓", "๔", "๕", "๖", "๗", "๘", "๙", "โอํ", "ฯ", "๚"],
    vedicSymbols: ["'", "ꣳ", "ꣴ"],
  },
  tibetan: {
    vowels: ["ཨ", "ཨཱ", "ཨི", "ཨཱི", "ཨུ", "ཨཱུ", "རྀ", "རཱྀ", "ལྀ", "ལཱྀ", "ཨེ", "ཨེ", "ཨཻ", "ཨོ", "ཨོ", "ཨཽ", "ཨེ", "ཨེ", "ཨོ", "ཨེ", "ཨུ", "ཨེ"],
    consonants: ["ཀ", "ཁ", "ག", "གྷ", "ང", "ཅ", "ཆ", "ཇ", "ཇྷ", "ཉ", "ཊ", "ཋ", "ཌ", "ཌྷ", "ཎ", "ཏ", "ཐ", "ད", "དྷ", "ན", "པ", "ཕ", "བ", "བྷ", "མ", "ཡ", "ར", "ལ", "ཝ", "ཤ", "ཥ", "ས", "ཧ", "ལ༹", "ཥ༹", "ར༹", "ན༹", "ཫ", "ཁ༹", "ག༹", "ཟ", "ཬ", "ཬྷ", "ཕ༹", "ཡ༹", "ྃཀ", "ྃཛ", "ྃཌ", "ྃད", "ྃབ", "ཙ", "ཚ", "ཏ", "ཞ", "ཝ", "ས", "ས", "ཧ", "ཏ", "ཧ", "ཟ", "ཟ", "ཟ", "ཛ"],
    vowelMarks: ["྄", "ཱ", "ི", "ཱི", "ུ", "ཱུ", "ྲྀ", "ྲཱྀ", "ླྀ", "ླཱྀ", "ེ", "ེ", "ཻ", "ོ", "ོ", "ཽ", "ེ", "ེ", "ོ", "ེ", "ུ", "ེ"],
    otherDiacritics: ["ྃ", "ཾ", "ཿ", "ཿ", "ྈ", "ྉ", "ྃ", "ཾ"],
    symbols: ["༠", "༡", "༢", "༣", "༤", "༥", "༦", "༧", "༨", "༩", "ༀ", "།", "༎"],
    vedicSymbols: ["྅", "ꣳ", "ꣴ"],
  },
  sinhala: {
    vowels: ["අ", "ආ", "ඉ", "ඊ", "උ", "ඌ", "ඍ", "ඎ", "ඏ", "ඐ", "එ", "ඒ", "ඓ", "ඔ", "ඕ", "ඖ", "ඇ", "ඈ", "ඔ", "එ", "උ", "ඈ"],
    consonants: ["ක", "ඛ", "ග", "ඝ", "ඞ", "ච", "ඡ", "ජ", "ඣ", "ඤ", "ට", "ඨ", "ඩ", "ඪ", "ණ", "ත", "ථ", "ද", "ධ", "න", "ප", "ඵ", "බ", "භ", "ම", "ය", "ර", "ල", "ව", "ශ", "ෂ", "ස", "හ", "ළ", "ළ", "ර", "න", "ක", "ඛ", "ග", "ජ", "ඩ", "ඪ", "ෆ", "ය", "ඟ", "ඦ", "ඬ", "ඳ", "ඹ", "ච", "ඡ", "ත", "ජ", "ව", "ස", "ස", "හ", "ත", "හ", "ජ", "ජ", "ජ", "ජ"],
    vowelMarks: ["්", "ා", "ි", "ී", "ු", "ූ", "ෘ", "ෲ", "ෟ", "ෳ", "ෙ", "ේ", "ෛ", "ො", "ෝ", "ෞ", "ැ", "ෑ", "ො", "ෙ", "ු", "ෑ"],
    otherDiacritics: ["ං", "ං", "ඃ", "ඃ", "ඃ", "ඃ", "ං", "ං"],
    symbols: ["෦", "෧", "෨", "෩", "෪", "෫", "෬", "෭", "෮", "෯", "ඕං", "।", "॥"],
    vedicSymbols: ["(අ)", "ꣳ", "ꣴ"],
  },
  devanagari: {
    vowels: ["अ", "आ", "इ", "ई", "उ", "ऊ", "ऋ", "ॠ", "ऌ", "ॡ", "ऎ", "ए", "ऐ", "ऒ", "ओ", "औ", "ऍ", "एॕ", "ऑ", "ऍ", "उ", "एॕ"],
    consonants: ["क", "ख", "ग", "घ", "ङ", "च", "छ", "ज", "झ", "ञ", "ट", "ठ", "ड", "ढ", "ण", "त", "थ", "द", "ध", "न", "प", "फ", "ब", "भ", "म", "य", "र", "ल", "व", "श", "ष", "स", "ह", "ळ", "ऴ", "ऱ", "ऩ", "क़", "ख़", "ग़", "ज़", "ड़", "ढ़", "फ़", "य़", "ँˆग", "ँˆज", "ँˆड", "ँˆद", "ँˆब", "च़", "छ़", "त़", "झ़", "व़", "स", "स", "ह", "त", "ह", "ज़", "ज़", "ज़", "ज"],
    vowelMarks: ["्", "ा", "ि", "ी", "ु", "ू", "ृ", "ॄ", "ॢ", "ॣ", "ॆ", "े", "ै", "ॊ", "ो", "ौ", "ॅ", "ॕ", "ॉ", "ॅ", "ु", "ॕ"],
    otherDiacritics: ["ँ", "ं", "ः", "ः", "ᳵ", "ᳶ", "ँ", "ं"],
    symbols: ["०", "१", "२", "३", "४", "५", "६", "७", "८", "९", "ॐ", "।", "॥"],
    vedicSymbols: ["ऽ", "ꣳ", "ꣴ"],
  },
  telugu: {
    vowels: ["అ", "ఆ", "ఇ", "ఈ", "ఉ", "ఊ", "ఋ", "ౠ", "ఌ", "ౡ", "ఎ", "ఏ", "ఐ", "ఒ", "ఓ", "ఔ", "ఎ", "ఏ", "ఒ", "ఎ", "ఉ", "ఏ"],
    consonants: ["క", "ఖ", "గ", "ఘ", "ఙ", "చ", "ఛ", "జ", "ఝ", "ఞ", "ట", "ఠ", "డ", "ఢ", "ణ", "త", "థ", "ద", "ధ", "న", "ప", "ఫ", "బ", "భ", "మ", "య", "ర", "ల", "వ", "శ", "ష", "స", "హ", "ళ", "ఴ", "ఱ", "న", "క", "ఖ", "గ", "జ", "డ", "ఢ", "ఫ", "య", "ఁˆగ", "ఁˆజ", "ఁˆడ", "ఁˆద", "ఁˆబ", "ౘ", "ఛ", "త", "జ", "వ", "స", "స", "హ", "త", "హ", "జ", "జ", "జ", "ౙ"],
    vowelMarks: ["్", "ా", "ి", "ీ", "ు", "ూ", "ృ", "ౄ", "ౢ", "ౣ", "ె", "ే", "ై", "ొ", "ో", "ౌ", "ె", "ే", "ొ", "ె", "ు", "ే"],
    otherDiacritics: ["ఀ", "ం", "ః", "ః", "ః", "ః", "ఁ", "ం"],
    symbols: ["౦", "౧", "౨", "౩", "౪", "౫", "౬", "౭", "౮", "౯", "ఓం", "।", "॥"],
    vedicSymbols: ["ఽ", "ꣳ", "ꣴ"],
  },
  kannada: {
    vowels: ["ಅ", "ಆ", "ಇ", "ಈ", "ಉ", "ಊ", "ಋ", "ೠ", "ಌ", "ೡ", "ಎ", "ಏ", "ಐ", "ಒ", "ಓ", "ಔ", "ಎ", "ಏ", "ಒ", "ಎ", "ಉ", "ಏ"],
    consonants: ["ಕ", "ಖ", "ಗ", "ಘ", "ಙ", "ಚ", "ಛ", "ಜ", "ಝ", "ಞ", "ಟ", "ಠ", "ಡ", "ಢ", "ಣ", "ತ", "ಥ", "ದ", "ಧ", "ನ", "ಪ", "ಫ", "ಬ", "ಭ", "ಮ", "ಯ", "ರ", "ಲ", "ವ", "ಶ", "ಷ", "ಸ", "ಹ", "ಳ", "ೞ", "ಱ", "ನ಼", "ಕ಼", "ಖ಼", "ಗ಼", "ಜ಼", "ಡ಼", "ಢ಼", "ಫ಼", "ಯ಼", "ಂˆಗ", "ಂˆಜ", "ಂˆಡ", "ಂˆದ", "ಂˆಬ", "ಚ", "ಛ", "ತ", "ಜ", "ವ", "ಸ", "ಸ", "ಹ", "ತ", "ಹ", "ಜ಼", "ಜ಼", "ಜ಼", "ಜ"],
    vowelMarks: ["್", "ಾ", "ಿ", "ೀ", "ು", "ೂ", "ೃ", "ೄ", "ೢ", "ೣ", "ೆ", "ೇ", "ೈ", "ೊ", "ೋ", "ೌ", "ೆ", "ೇ", "ೊ", "ೆ", "ು", "ೇ"],
    otherDiacritics: ["ಁ", "ಂ", "ಃ", "ಃ", "ೱ", "ೲ", "ಁ", "ಂ"],
    symbols: ["೦", "೧", "೨", "೩", "೪", "೫", "೬", "೭", "೮", "೯", "ಓಂ", "।", "॥"],
    vedicSymbols: ["ಽ", "ꣳ", "ꣴ"],
  },
  tamil: {
    vowels: ["அ", "ஆ", "இ", "ஈ", "உ", "ஊ", "ரு", "ரூ", "லு", "லூ", "எ", "ஏ", "ஐ", "ஒ", "ஓ", "ஔ", "எ", "ஏ", "ஒ", "எ", "உ", "ஏ"],
    consonants: ["க", "க²", "க³", "க⁴", "ங", "ச", "ச²", "ஜ", "ஜ²", "ஞ", "ட", "ட²", "ட³", "ட⁴", "ண", "த", "த²", "த³", "த⁴", "ந", "ப", "ப²", "ப³", "ப⁴", "ம", "ய", "ர", "ல", "வ", "ஶ", "ஷ", "ஸ", "ஹ", "ள", "ழ", "ற", "ன", "ஃக", "ஃக²", "ஃக³", "ஃஜ", "ஃட²", "ஃட³", "ஃப", "ஃய", "ங்ˆக³", "ஞ்ˆஜ³", "ண்ˆட³", "ந்ˆத³", "ம்ˆப³", "ச", "ச²", "த", "ஜ", "வ", "ஸ", "ஸ", "ஹ", "த", "ஹ", "ஃஜ", "ஃஜ", "ஃஜ", "ஜ"],
    vowelMarks: ["்", "ா", "ி", "ீ", "ு", "ூ", "்ரு", "்ரூ", "்லு", "்லூ", "ெ", "ே", "ை", "ொ", "ோ", "ௌ", "ெ", "ே", "ொ", "ெ", "ு", "ே"],
    otherDiacritics: ["ம்ˮ", "ம்ʼ", "꞉", "ஃ", "꞉", "꞉", "ம்ˮ", "ம்ʼ"],
    symbols: ["௦", "௧", "௨", "௩", "௪", "௫", "௬", "௭", "௮", "௯", "ௐ", "।", "॥"],
    vedicSymbols: ["(அ)", "ꣳ", "ꣴ"],
  },
  malayalam: {
    vowels: ["അ", "ആ", "ഇ", "ഈ", "ഉ", "ഊ", "ഋ", "ൠ", "ഌ", "ൡ", "എ", "ഏ", "ഐ", "ഒ", "ഓ", "ഔ", "എ", "ഏ", "ഒ", "എ", "ഉ്", "ഏ"],
    consonants: ["ക", "ഖ", "ഗ", "ഘ", "ങ", "ച", "ഛ", "ജ", "ഝ", "ഞ", "ട", "ഠ", "ഡ", "ഢ", "ണ", "ത", "ഥ", "ദ", "ധ", "ന", "പ", "ഫ", "ബ", "ഭ", "മ", "യ", "ര", "ല", "വ", "ശ", "ഷ", "സ", "ഹ", "ള", "ഴ", "റ", "ഩ", "ക", "ഖ", "ഗ", "ജ", "ഡ", "ഢ", "ഫ", "യ", "ംˆഗ", "ംˆജ", "ംˆഡ", "ംˆദ", "ംˆബ", "ച", "ഛ", "ഺ", "ജ", "വ", "സ", "സ", "ഹ", "ത", "ഹ", "ജ", "ജ", "ജ", "ജ"],
    vowelMarks: ["്", "ാ", "ി", "ീ", "ു", "ൂ", "ൃ", "ൄ", "ൢ", "ൣ", "െ", "േ", "ൈ", "ൊ", "ോ", "ൗ", "െ", "േ", "ൊ", "െ", "ു്", "േ"],
    otherDiacritics: ["ഁ", "ം", "ഃ", "ഃ", "ഃ", "ഃ", "ഁ", "ം"],
    symbols: ["൦", "൧", "൨", "൩", "൪", "൫", "൬", "൭", "൮", "൯", "ഓം", "।", "॥"],
    vedicSymbols: ["ഽ", "ꣳ", "ꣴ"],
  },
  bengali: {
    vowels: ["অ", "আ", "ই", "ঈ", "উ", "ঊ", "ঋ", "ৠ", "ঌ", "ৡ", "এ", "এ", "ঐ", "ও", "ও", "ঔ", "অ্যা", "অ্যা", "অ", "অ্যা", "উ", "অ্যা"],
    consonants: ["ক", "খ", "গ", "ঘ", "ঙ", "চ", "ছ", "জ", "ঝ", "ঞ", "ট", "ঠ", "ড", "ঢ", "ণ", "ত", "থ", "দ", "ধ", "ন", "প", "ফ", "ব", "ভ", "ম", "য", "র", "ল", "ভ়", "শ", "ষ", "স", "হ", "ল়", "ষ়", "র়", "ন়", "ক়", "খ়", "গ়", "জ়", "ড়", "ঢ়", "ফ়", "য়", "ঁˆগ", "ঁˆজ", "ঁˆড", "ঁˆদ", "ঁˆব", "চ", "ছ", "ত", "ঝ়", "ৱ়", "স", "স", "হ", "ত", "হ", "জ়", "জ়", "জ়", "জ"],
    vowelMarks: ["্", "া", "ি", "ী", "ু", "ূ", "ৃ", "ৄ", "ৢ", "ৣ", "ে", "ে", "ৈ", "ো", "ো", "ৌ", "্যা", "্যা", "", "্যা", "ু", "্যা"],
    otherDiacritics: ["ঁ", "ং", "ঃ", "ঃ", "ঃ", "ঃ", "ঁ", "ং"],
    symbols: ["০", "১", "২", "৩", "৪", "৫", "৬", "৭", "৮", "৯", "ওঁ", "।", "॥"],
    vedicSymbols: ["ঽ", "ꣳ", "ꣴ"],
  },
  gujarati: {
    vowels: ["અ", "આ", "ઇ", "ઈ", "ઉ", "ઊ", "ઋ", "ૠ", "ઌ", "ૡ", "એ", "એ", "ઐ", "ઓ", "ઓ", "ઔ", "ઍ", "એ", "ઑ", "ઍ", "ઉ", "એ"],
    consonants: ["ક", "ખ", "ગ", "ઘ", "ઙ", "ચ", "છ", "જ", "ઝ", "ઞ", "ટ", "ઠ", "ડ", "ઢ", "ણ", "ત", "થ", "દ", "ધ", "ન", "પ", "ફ", "બ", "ભ", "મ", "ય", "ર", "લ", "વ", "શ", "ષ", "સ", "હ", "ળ", "ળ઼", "ર઼", "ન઼", "ક઼", "ખ઼", "ગ઼", "જ઼", "ડ઼", "ઢ઼", "ફ઼", "ય઼", "ઁˆગ", "ઁˆજ", "ઁˆડ", "ઁˆદ", "ઁˆબ", "ચ", "છ", "ત", "જ", "વ", "સ", "સ", "હ", "ત", "હ", "જ઼", "જ઼", "જ઼", "જ"],
    vowelMarks: ["્", "ા", "િ", "ી", "ુ", "ૂ", "ૃ", "ૄ", "ૢ", "ૣ", "ે", "ે", "ૈ", "ો", "ો", "ૌ", "ૅ", "ે", "ૉ", "ૅ", "ુ", "ે"],
    otherDiacritics: ["ઁ", "ં", "ઃ", "ઃ", "ઃ", "ઃ", "ઁ", "ં"],
    symbols: ["૦", "૧", "૨", "૩", "૪", "૫", "૬", "૭", "૮", "૯", "ૐ", "।", "॥"],
    vedicSymbols: ["ઽ", "ꣳ", "ꣴ"],
  },
  gurmukhi: {
    vowels: ["ਅ", "ਆ", "ਇ", "ਈ", "ਉ", "ਊ", "ਰੁ", "ਰੂ", "ਲੁ", "ਲੂ", "ਏ", "ਏ", "ਐ", "ਓ", "ਓ", "ਔ", "ਐ", "ਐ", "ਔ", "ਏ", "ਉ", "ਐ"],
    consonants: ["ਕ", "ਖ", "ਗ", "ਘ", "ਙ", "ਚ", "ਛ", "ਜ", "ਝ", "ਞ", "ਟ", "ਠ", "ਡ", "ਢ", "ਣ", "ਤ", "ਥ", "ਦ", "ਧ", "ਨ", "ਪ", "ਫ", "ਬ", "ਭ", "ਮ", "ਯ", "ਰ", "ਲ", "ਵ", "ਸ਼", "ਸ਼਼", "ਸ", "ਹ", "ਲ਼", "ਲ਼਼", "ਰ਼", "ਨ਼", "ਕ਼", "ਖ਼", "ਗ਼", "ਜ਼", "ੜ", "ਢ਼", "ਫ਼", "ਯ਼", "ਁˆਗ", "ਁˆਜ", "ਁˆਡ", "ਁˆਦ", "ਁˆਬ", "ਚ", "ਛ", "ਤ", "ਝ਼", "ਵ", "ਸ", "ਸ", "ਹ", "ਤ", "ਹ", "ਜ਼", "ਜ਼", "ਜ਼", "ਜ"],
    vowelMarks: ["੍", "ਾ", "ਿ", "ੀ", "ੁ", "ੂ", "੍ਰੁ", "੍ਰੂ", "੍ਲੁ", "੍ਲੂ", "ੇ", "ੇ", "ੈ", "ੋ", "ੋ", "ੌ", "ੈ", "ੈ", "ੌ", "ੇ", "ੁ", "ੈ"],
    otherDiacritics: ["ਁ", "ਂ", "ਃ", "ਃ", "ਃ", "ਃ", "ਁ", "ੰ"],
    symbols: ["੦", "੧", "੨", "੩", "੪", "੫", "੬", "੭", "੮", "੯", "ੴ", "।", "॥"],
    vedicSymbols: ["(ਅ)", "ꣳ", "ꣴ"],
  },
  oriya: {
    vowels: ["ଅ", "ଆ", "ଇ", "ଈ", "ଉ", "ଊ", "ଋ", "ୠ", "ଌ", "ୡ", "ଏ", "ଏ", "ଐ", "ଓ", "ଓ", "ଔ", "ଏ", "ଏ", "ଓ", "ଏ", "ଉ", "ଏ"],
    consonants: ["କ", "ଖ", "ଗ", "ଘ", "ଙ", "ଚ", "ଛ", "ଜ", "ଝ", "ଞ", "ଟ", "ଠ", "ଡ", "ଢ", "ଣ", "ତ", "ଥ", "ଦ", "ଧ", "ନ", "ପ", "ଫ", "ବ", "ଭ", "ମ", "ଯ", "ର", "ଲ", "ଵ", "ଶ", "ଷ", "ସ", "ହ", "ଳ", "ଳ଼", "ର଼", "ନ଼", "କ଼", "ଖ଼", "ଗ଼", "ଜ଼", "ଡ଼", "ଢ଼", "ଫ଼", "ୟ", "ଁˆଗ", "ଁˆଜ", "ଁˆଡ", "ଁˆଦ", "ଁˆବ", "ଚ", "ଛ", "ତ", "ଜ", "ୱ", "ସ", "ସ", "ହ", "ତ", "ହ", "ଜ଼", "ଜ଼", "ଜ଼", "ଜ"],
    vowelMarks: ["୍", "ା", "ି", "ୀ", "ୁ", "ୂ", "ୃ", "ୄ", "ୢ", "ୣ", "େ", "େ", "ୈ", "ୋ", "ୋ", "ୌ", "େ", "େ", "ୋ", "େ", "ୁ", "େ"],
    otherDiacritics: ["ଁ", "ଂ", "ଃ", "ଃ", "ଃ", "ଃ", "ଁ", "ଂ"],
    symbols: ["୦", "୧", "୨", "୩", "୪", "୫", "୬", "୭", "୮", "୯", "ଓଁ", "।", "॥"],
    vedicSymbols: ["ଽ", "ꣳ", "ꣴ"],
  },
  iast: {
    vowels: ["a", "ā", "i", "ī", "u", "ū", "ṛ", "ṝ", "ḷ", "ḹ", "ĕ", "e", "ai", "ŏ", "o", "au", "æ", "ǣ", "ô", "ĕ", "u", "ǣ"],
    consonants: ["k", "kh", "g", "gh", "ṅ", "c", "ch", "j", "jh", "ñ", "ṭ", "ṭh", "ḍ", "ḍh", "ṇ", "t", "th", "d", "dh", "n", "p", "ph", "b", "bh", "m", "y", "r", "l", "v", "ś", "ṣ", "s", "h", "l̤", "ḻ", "ṟ", "ṉ", "q", "k͟h", "ġ", "z", "r̤", "r̤h", "f", "ẏ", "n̆g", "n̆j", "n̆ḍ", "n̆d", "m̆b", "c", "ch", "t", "j", "v", "s", "s", "h", "t", "h", "z", "z", "z", "j"],
    vowelMarks: ["", "ā", "i", "ī", "u", "ū", "ṛ", "ṝ", "ḷ", "ḹ", "ĕ", "e", "ai", "ŏ", "o", "au", "æ", "ǣ", "ô", "ĕ", "u", "ǣ"],
    otherDiacritics: ["m̐", "ṃ", "ḥ", "ḵ", "ḥ", "ḥ", "m̐", "ṃ"],
    symbols: ["0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "oṃ", ".", ".."],
    vedicSymbols: ["'", "gͫ", "gͫ̄"],
  },
  itrans: {
    vowels: ["a", "aa", "i", "ee", "u", "uu", "R^i", "R^I", "L^i", "L^I", "e", "E", "ai", "o", "O", "au", "e.c", "a.C", "A.c", "e", "u", "a.C"],
    consonants: ["k", "kh", "g", "gh", "~N", "ch", "chh", "j", "jh", "~n", "T", "Th", "D", "Dh", "N", "t", "th", "d", "dh", "n", "p", "ph", "b", "bh", "m", "y", "r", "l", "v", "sh", "Sh", "s", "h", "L", "zh", "R", "^n", "q", "K", "G", "z", ".D", ".Dh", "f", "Y", "n*g", "n*j", "n*D", "n*d", "m*b", "ch", "chh", "t", "j", "v", "s", "s", "h", "t", "h", "z", "z", "z", "j"],
    vowelMarks: ["", "aa", "i", "ee", "u", "uu", "R^i", "R^I", "L^i", "L^I", "e", "E", "ai", "o", "O", "au", "e.c", "a.C", "A.c", "e", "u", "a.C"],
    otherDiacritics: [".N", "M", "H", "K^", "H", "H", ".N", "M"],
    symbols: ["0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "OM", ".", ".."],
    vedicSymbols: [".a", "gum", "ggum"],
  },
  hk: {
    vowels: ["a", "A", "i", "I", "u", "U", "R", "RR", "lR", "lRR", "E", "e", "ai", "O", "o", "au", "aE", "AE", "aO", "E", "u", "AE"],
    consonants: ["k", "kh", "g", "gh", "G", "c", "ch", "j", "jh", "J", "T", "Th", "D", "Dh", "N", "t", "th", "d", "dh", "n", "p", "ph", "b", "bh", "m", "y", "r", "l", "v", "z", "S", "s", "h", "L", "Z", "r2", "n2", "q", "qh", "g2", "z2", "r3", "r3h", "f", "Y", "n*g", "n*j", "n*D", "n*d", "m*b", "c", "ch", "t", "j", "v", "s", "s", "h", "t", "h", "z2", "z2", "z2", "j"],
    vowelMarks: ["", "A", "i", "I", "u", "U", "R", "RR", "lR", "lRR", "E", "e", "ai", "O", "o", "au", "aE", "AE", "aO", "E", "u", "AE"],
    otherDiacritics: ["~", "M", "H", "K", "H", "H", "~", "M"],
    symbols: ["0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "oM", ".", ".."],
    vedicSymbols: ["'", "gͫ", "gͫ̄"],
  },
  iso: {
    vowels: ["a", "ā", "i", "ī", "u", "ū", "r̥", "r̥̄", "l̥", "l̥̄", "e", "ē", "ai", "o", "ō", "au", "æ", "ǣ", "ô", "ê", "ŭ", "ê̄"],
    consonants: ["k", "kh", "g", "gh", "ṅ", "c", "ch", "j", "jh", "ñ", "ṭ", "ṭh", "ḍ", "ḍh", "ṇ", "t", "th", "d", "dh", "n", "p", "ph", "b", "bh", "m", "y", "r", "l", "v", "ś", "ṣ", "s", "h", "ḷ", "ḻ", "ṟ", "ṉ", "q", "k͟h", "ġ", "z", "ṛ", "ṛh", "f", "ẏ", "n̆g", "n̆j", "n̆ḍ", "n̆d", "m̆b", "ĉ", "ĉh", "ṯ", "ž", "w", "s̱", "s̤", "h̤", "t̤", "ʻ", "ẕ", "ż", "ẓ", "dz"],
    vowelMarks: ["", "ā", "i", "ī", "u", "ū", "r̥", "r̥̄", "l̥", "l̥̄", "e", "ē", "ai", "o", "ō", "au", "æ", "ǣ", "ô", "ê", "ŭ", "ê̄"],
    otherDiacritics: ["m̐", "ṁ", "ḥ", "ḵ", "ẖ", "ḫ", "n̆", "ṃ"],
    symbols: ["0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "ōṁ", ".", ".."],
    vedicSymbols: ["’", "gͫ", "gͫ̄"],
  },
  slp1: {
    vowels: ["a", "A", "i", "I", "u", "U", "f", "F", "x", "X", "e1", "e", "E", "o1", "o", "O", "e2", "e4", "o2", "e1", "u", "e4"],
    consonants: ["k", "K", "g", "G", "N", "c", "C", "j", "J", "Y", "w", "W", "q", "Q", "R", "t", "T", "d", "D", "n", "p", "P", "b", "B", "m", "y", "r", "l", "v", "S", "z", "s", "h", "L", "L0", "r2", "n2", "k0", "K0", "g0", "j0", "q0", "Q0", "P0", "Y0", "n*g", "n*j", "n*q", "n*d", "m*b", "c", "C", "t", "j", "v", "s", "s", "h", "t", "h", "j0", "j0", "j0", "j"],
    vowelMarks: ["", "A", "i", "I", "u", "U", "f", "F", "x", "X", "e1", "e", "E", "o1", "o", "O", "e2", "e4", "o2", "e1", "u", "e4"],
    otherDiacritics: ["~", "M", "H", "K", "H", "H", "~", "M"],
    symbols: ["0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "oM", ".", ".."],
    vedicSymbols: ["'", "gͫ", "gͫ̄"],
  },
  indlish: {
    vowels: ["a", "A", "i", "I", "u", "U", "r̥", "r̥", "l̥", "l̥", "e", "E", "ai", "o", "O", "au", "ă", "̆e", "̆o", "e", "u", "̆e"],
    consonants: ["k", "kh", "g", "gh", "n", "ch", "CH", "j", "jh", "n", "T", "TH", "D", "DH", "N", "t", "th", "d", "dh", "n", "p", "ph", "b", "bh", "m", "y", "r", "l", "v", "sh", "sh", "s", "h", "L", "zh", "R", "n", "q", "qh", "gh", "z", "D", "Dh", "f", "y", "ng", "nj", "nD", "nd", "mb", "ch", "CH", "t", "j", "v", "s", "s", "h", "t", "h", "z", "z", "z", "j"],
    vowelMarks: ["", "A", "i", "I", "u", "U", "r̥", "r̥", "l̥", "l̥", "e", "E", "ai", "o", "O", "au", "ă", "̆e", "̆o", "e", "u", "̆e"],
    otherDiacritics: ["m̐", "ṁ", "ḥ", "ḵ", "ḥ", "ḥ", "m̐", "ṁ"],
    symbols: ["0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "OM", "!", "!!"],
    vedicSymbols: ["~", "gum", "gum"],
  },
  velthuis: {
    vowels: ["a", "aa", "i", "ii", "u", "uu", ".r", ".rr", ".l", ".ll", "E", "e", "ai", ".O", "o", "au", "aE", "AE", "aO", "E", "u", "AE"],
    consonants: ["k", "kh", "g", "gh", "\"n", "c", "ch", "j", "jh", "~n", ".t", ".th", ".d", ".dh", ".n", "t", "th", "d", "dh", "n", "p", "ph", "b", "bh", "m", "y", "r", "l", "v", "\"s", ".s", "s", "h", "L", "_l", "_r", "_n", "q", "_kh", ".g", "z", "..r", "..rh", "f", ".y", "n*g", "n*j", "n*.d", "n*d", "m*b", "c", "ch", "t", "j", "v", "s", "s", "h", "t", "h", "z", "z", "z", "j"],
    vowelMarks: ["", "aa", "i", "ii", "u", "uu", ".r", ".rr", ".l", ".ll", "E", "e", "ai", ".O", "o", "au", "aE", "AE", "aO", "E", "u", "AE"],
    otherDiacritics: ["~m", ".m", ".h", "_k", ".h", ".h", "~m", ".m"],
    symbols: ["0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "O", ".", ".."],
    vedicSymbols: [".a", "gͫ", "gͫ̄"],
  },
  wx: {
    vowels: ["a", "A", "i", "I", "u", "U", "q", "qV", "L", "LV", "eV", "e", "E", "oV", "o", "O", "aEV", "AEV", "aOV", "eV", "u", "AEV"],
    consonants: ["k", "K", "g", "G", "f", "c", "C", "j", "J", "F", "t", "T", "d", "D", "N", "w", "W", "x", "X", "n", "p", "P", "b", "B", "m", "y", "r", "l", "v", "S", "R", "s", "h", "lY", "lYY", "rY", "nY", "kY", "KY", "GY", "zY", "dY", "DY", "PY", "yY", "nYYg", "nYYj", "nYYd", "nYYx", "mYYb", "c", "C", "w", "j", "v", "s", "s", "h", "w", "h", "zY", "zY", "zY", "j"],
    vowelMarks: ["", "A", "i", "I", "u", "U", "q", "qV", "L", "LV", "eV", "e", "E", "oV", "o", "O", "aEV", "AEV", "aOV", "eV", "u", "AEV"],
    otherDiacritics: ["z", "M", "H", "kYY", "H", "H", "z", "M"],
    symbols: ["0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "oM", ".", ".."],
    vedicSymbols: ["Z", "gͫ", "gͫ̄"],
  },
  titus: {
    vowels: ["a", "ā", "i", "ī", "u", "ū", "r̥", "r̥̄", "l̥", "l̥̄", "ĕ", "e", "ai", "ŏ", "o", "au", "æ", "ǣ", "ô", "ĕ", "u", "ǣ"],
    consonants: ["k", "kʰ", "g", "gʰ", "ṅ", "c", "cʰ", "j", "jʰ", "ñ", "ṭ", "ṭʰ", "ḍ", "ḍʰ", "ṇ", "t", "tʰ", "d", "dʰ", "n", "p", "pʰ", "b", "bʰ", "m", "y", "r", "l", "v", "ś", "ṣ", "s", "h", "ḷ", "ḻ", "ṟ", "ṉ", "q", "k͟ʰ", "ġ", "z", "ṛ", "ṛʰ", "f", "ẏ", "n̆g", "n̆j", "n̆ḍ", "n̆d", "m̆b", "c", "cʰ", "t", "j", "v", "s", "s", "h", "t", "h", "z", "z", "z", "j"],
    vowelMarks: ["", "ā", "i", "ī", "u", "ū", "r̥", "r̥̄", "l̥", "l̥̄", "ĕ", "e", "ai", "ŏ", "o", "au", "æ", "ǣ", "ô", "ĕ", "u", "ǣ"],
    otherDiacritics: ["m̐", "ṃ", "ḥ", "ḵ", "ḥ", "ḥ", "m̐", "ṃ"],
    symbols: ["0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "oṃ", ".", ".."],
    vedicSymbols: ["'", "gͫ", "gͫ̄"],
  },
  urdu: {
    vowels: ["اَ", "آ", "اِ", "اِی", "اُ", "اُو", "رِ", "رِی", "لِ", "لِی", "اے", "اے", "اَے", "او", "او", "اَو", "اَے", "اَے", "اَو", "اَے", "اُ", "اَے"],
    consonants: ["ک", "کھ", "گ", "گھ", "ن٘", "چ", "چھ", "ج", "جھ", "ڃ", "ٹ", "ٹھ", "ڈ", "ڈھ", "ݨ", "ت", "تھ", "د", "دھ", "ن", "پ", "پھ", "ب", "بھ", "م", "ی", "ر", "ل", "و", "ش", "ش", "س", "ہ", "لؕ", "ل", "ر", "ن", "ق", "خ", "غ", "ز", "ڑ", "ڑھ", "ف", "ی", "نگ", "نج", "نڈ", "ند", "مب", "تس", "تسھ", "ت", "ژ", "و", "ث", "ص", "ح", "ط", "ع", "ذ", "ض", "ظ", "دز"],
    vowelMarks: ["ْ", "ا", "ِ", "ِی", "ُ", "ُو", "رِ", "رِی", "لِ", "لِی", "ے", "ے", "َے", "و", "و", "َو", "َے", "َے", "َو", "َے", "ُ", "َے"],
    otherDiacritics: ["ں", "ں", "ہ", "ہ", "ہ", "ہ", "ں", "ں"],
    symbols: ["۰", "۱", "۲", "۳", "۴", "۵", "۶", "۷", "۸", "۹", "اوم", "۔", "۔۔"],
    vedicSymbols: ["’", "گم", "گم"],
    inherentVowel: 'َ',
  },
};

const VEDIC_KEYBOARD_ACCENTS = ["॒", "॑", "᳚"];
const ROMAN_KEYBOARD_ACCENTS = ["̱", "̍", "̎"];

// ASCII input tokens follow the same character order as KEYBOARD_MAPS.
// Indlish-inspired spellings with distinct tokens for every keyboard slot.
const ASCII_KEYBOARD_TOKENS = {
  vowels: ['a', 'A', 'i', 'I', 'u', 'U', 'R^i', 'R^I', 'L^i', 'L^I', 'e', 'E', 'ai', 'o', 'O', 'au', 'ae', 'aE', 'ao', 'e^', 'u^', 'e^^'],
  vowelMarks: ['`', '.A', '.i', '.I', '.u', '.U', '.R^i', '.R^I', '.L^i', '.L^I', '.e', '.E', '.ai', '.o', '.O', '.au', '.ae', '.aE', '.ao', '.e^', '.u^', '.e^^'],
  consonants: ['k', 'kh', 'g', 'gh', 'ng', 'ch', 'CH', 'j', 'jh', 'ny', 'T', 'TH', 'D', 'DH', 'N', 't', 'th', 'd', 'dh', 'n', 'p', 'ph', 'b', 'bh', 'm', 'y', 'r', 'l', 'v', 'sh', 'Sh', 's', 'h', 'L', 'zh', 'R', 'nh', 'q', 'qh', 'G', 'z', 'Rd', 'Rdh', 'f', 'Y', '~ng', '~nj', '~nD', '~nd', '~mb', 'ts', 'tsh', 't_', 'jz', 'w', 's_', 's:', 'h:', 't:', 'ayn', 'z_', 'z.', 'z:', 'dz'],
  otherDiacritics: ['~M', 'M', 'H', '.H', 'Hk', 'Hp', 'n^', 'M^'],
  symbols: ['0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'OM', '!', '!!', "'", '~g', '~G'],
  accents: ['^1', '^2', '^3']
};

// Fixed reference lookup populated with Devanagari characters (editable manually)
const KEYBOARD_REFERENCE_MAP = {
  "symbols": [
    "०", "१", "२", "३", "४", "५", "६", "७", "८", "९",
    "ॐ", "।", "॥", "ऽ", "ꣳ", "ꣴ"
  ],
  "otherDiacritics": [
    "ँ", "ं", "ः", "ஃ"
  ],
  "accents": [
    "॒", "॑", "᳚"
  ],
  "vowels": [
    "अ", "आ", "इ", "ई", "उ", "ऊ", "ऋ", "ॠ", "ऌ", "ॡ",
    "ऎ", "ए", "ऐ", "ऒ", "ओ", "औ", "ऍ", "एॕ", "ऑ"
  ],
  "vowelMarks": [
    "्", "ा", "ि", "ी", "ु", "ू", "ृ", "ॄ", "ॢ", "ॣ",
    "ॆ", "े", "ै", "ॊ", "ो", "ौ", "ॅ", "ॕ", "ॉ"
  ],
  "consonants": [
    // Ka-varga (Velar)
    "क", "ख", "ग", "घ", "ङ",
    // Cha-varga (Palatal)
    "च", "छ", "ज", "झ", "ञ",
    // Retroflex Ta-varga
    "ट", "ठ", "ड", "ढ", "ण",
    // Dental Ta-varga
    "त", "थ", "द", "ध", "न",
    // Pa-varga (Labial)
    "प", "फ", "ब", "भ", "म",
    // Antastha & Ushman (Semi-vowels & Sibilants / Aspirate)
    "य", "र", "ल", "व", "श", "ष", "स", "ह", "ळ",
    // Dravidian / Vedic & Nukta Additions (ऴ-य़)
    "ऴ", "ऱ", "ऩ", "क़", "ख़", "ग़", "ज़", "ड़", "ढ़", "फ़", "य़",
    // Prenasalized Consonants
    "ඟ", "ඦ", "ඬ", "ඳ", "ඹ"
  ]
};

// Change one palette name to recolor a category in both themes (including its toggle dot).
const KEYBOARD_COLORS = {
  "number": "teal",
  "symbol": "peach",
  "diacritic": "sky",
  "accent": "rose",
  "vowel": "purple",
  "vowelmark": "gold",
  "varga-1": "pink",
  "varga-2": "cyan",
  "varga-3": "orange",
  "varga-4": "indigo",
  "varga-5": "coral",
  "varga": "green",
  "semivowel": "amber",
  "zha-yya": "slate",
  "prenasal": "emerald",
  "additional": "green"
};

// Add a script here; source/target default to true. Fonts default to ROMAN_FONTS.
// Indic keyboard choices are derived from kind + keyboard, in catalog order.
// Example: assamese: { label: 'Assamese', kind: 'indic', lang: 'as',
//   fonts: ['Noto Serif Bengali'], keyboard: KEYBOARD_MAPS.bengali }.
// The active engine must support the ID; unavailable entries remain disabled.
const SCRIPT_CATALOG = {
  indic: {
    "label": "Mixed Indic",
    "target": false,
  },
  separatorIndic: {
    label: '── Indic ──', separator: true,
  },
  assamese: {
    supportsVedicAccents: true,
    "label": "Assamese", "kind": "indic",
    "keyboard": KEYBOARD_MAPS.assamese,
    "fonts": ["Noto Serif Bengali", "Noto Sans Bengali"], "lang": "as",
    "sample": "অগ্নিমীळে নমঃ",
  },
  bengali: {
    supportsVedicAccents: true,
    "label": "Bengali",
    "kind": "indic",
    "fonts": ["Noto Serif Bengali", "Noto Sans Bengali", "Tiro Bangla"],
    "lang": "sa-Beng",
    "sample": "অ॒গ্নিমী॑ळে নমঃ",
    "keyboard": KEYBOARD_MAPS.bengali,
  },
  burmese: {
    supportsVedicAccents: false,
    "label": "Burmese", "kind": "indic",
    "keyboard": KEYBOARD_MAPS.burmese,
    "fonts": ["Noto Serif Myanmar", "Noto Sans Myanmar"], "lang": "my",
    "sample": "အဂ္နိမီဠေ နမಃ",
  },
  devanagari: {
    supportsVedicAccents: true,
    "label": "Devanagari",
    "kind": "indic",
    "fonts": ["Noto Serif Devanagari", "Noto Sans Devanagari", "Tiro Devanagari Sanskrit"],
    "lang": "sa-Deva",
    "keyboard": KEYBOARD_MAPS.devanagari,
  },
  grantha: {
    supportsVedicAccents: false,
    "label": "Grantha", "kind": "indic",
    "keyboard": KEYBOARD_MAPS.grantha,
    "fonts": ["Noto Sans Grantha"], "lang": "sa-Gran",
    "sample": "𑍐 𑌨𑌮𑌃 𑌶𑌿𑌵𑌾𑌯",
  },
  gujarati: {
    supportsVedicAccents: true,
    "label": "Gujarati",
    "kind": "indic",
    "fonts": ["Noto Serif Gujarati", "Noto Sans Gujarati"],
    "lang": "sa-Gujr",
    "sample": "ૐ નમઃ શિવાય",
    "keyboard": KEYBOARD_MAPS.gujarati,
  },
  gurmukhi: {
    supportsVedicAccents: true,
    "label": "Gurmukhi",
    "kind": "indic",
    "fonts": ["Noto Serif Gurmukhi", "Noto Sans Gurmukhi"],
    "lang": "sa-Guru",
    "sample": "ੴ ਸਤਿ ਨਾਮੁ",
    "keyboard": KEYBOARD_MAPS.gurmukhi,
  },
  kannada: {
    supportsVedicAccents: true,
    "label": "Kannada",
    "kind": "indic",
    "fonts": ["Noto Serif Kannada", "Noto Sans Kannada", "Tiro Kannada"],
    "lang": "sa-Knda",
    "sample": "ಅ॒ಗ್ನಿಮೀ॑ಳೇ । ತ್ವ॒ ತ್ಯ॒ ತ್ವೋ॒ ತ್ಯೋ॒ ತ್ವಂ᳚",
    "keyboard": KEYBOARD_MAPS.kannada,
  },
  khmer: {
    supportsVedicAccents: false,
    "label": "Khmer", "kind": "indic",
    "keyboard": KEYBOARD_MAPS.khmer,
    "fonts": ["Noto Serif Khmer", "Noto Sans Khmer"], "lang": "km",
    "sample": "អគ្និមីឡេ នមៈ",
  },
  malayalam: {
    supportsVedicAccents: true,
    "label": "Malayalam",
    "kind": "indic",
    "fonts": ["Noto Serif Malayalam", "Noto Sans Malayalam"],
    "lang": "sa-Mlym",
    "sample": "ഓം നമഃ ശിവായ",
    "keyboard": KEYBOARD_MAPS.malayalam,
  },
  oriya: {
    supportsVedicAccents: true,
    "label": "Odia",
    "kind": "indic",
    "fonts": ["Noto Serif Oriya", "Noto Sans Oriya"],
    "lang": "sa-Orya",
    "sample": "ଓଁ ନମଃ ଶିବାୟ",
    "keyboard": KEYBOARD_MAPS.oriya,
  },
  sinhala: {
    supportsVedicAccents: false,
    "label": "Sinhala",
    "kind": "indic",
    "fonts": ["Noto Serif Sinhala", "Noto Sans Sinhala"],
    "lang": "sa-Sinh",
    "sample": "ඕං නමඃ ශිවාය",
    "keyboard": KEYBOARD_MAPS.sinhala,
  },
  tamil: {
    supportsVedicAccents: true,
    "label": "Tamil",
    "kind": "indic",
    "fonts": ["Noto Serif Tamil", "Noto Sans Tamil", "Tiro Tamil"],
    "lang": "sa-Taml",
    "sample": "ஓம் நமஃ ஶிவாய",
    "keyboard": KEYBOARD_MAPS.tamil,
  },
  telugu: {
    supportsVedicAccents: true,
    "label": "Telugu",
    "kind": "indic",
    "fonts": ["Noto Serif Telugu", "Noto Sans Telugu", "Tiro Telugu"],
    "lang": "sa-Telu",
    "sample": "అ॒గ్నిమీ॑ళే । త్వ॒ త్య॒ త్వో॒ త్యో॒ త్వం᳚",
    "keyboard": KEYBOARD_MAPS.telugu,
  },
  thai: {
    supportsVedicAccents: false,
    "label": "Thai", "kind": "indic",
    "keyboard": KEYBOARD_MAPS.thai,
    "fonts": ["Noto Serif Thai", "Noto Sans Thai"], "lang": "th",
    "sample": "อัคนิมีเฬ นมะ",
  },
  tibetan: {
    supportsVedicAccents: false,
    "label": "Tibetan", "kind": "indic",
    "keyboard": KEYBOARD_MAPS.tibetan,
    "fonts": ["Noto Serif Tibetan", "Noto Sans Tibetan"], "lang": "bo",
    "sample": "ཨགྣིམཱིལེ ནམཿ",
  },
  separatorRoman: {
    label: '── Roman ──', separator: true,
  },
  hk: {
    supportsVedicAccents: true,
    kind: 'roman',
    "label": "Harvard-Kyoto",
    "sample": "zrI gurubhyo namaH . hariH oM",
    "keyboard": KEYBOARD_MAPS.hk,
  },
  iast: {
    supportsVedicAccents: true,
    kind: 'roman',
    "label": "IAST",
    "sample": "śrī̍ gurubhyo̱ namaḥ̎ । agnim īḷe purohitaṃ",
    "keyboard": KEYBOARD_MAPS.iast,
  },
  indlish: {
    supportsVedicAccents: true,
    kind: 'roman',
    "label": "Indlish",
    "source": false,
    "keyboard": KEYBOARD_MAPS.indlish,
  },
  ipa: {
    supportsVedicAccents: true,
    kind: 'roman',
    "label": "IPA",
    "keyboard": KEYBOARD_MAPS.ipa,
    "sample": "ɐɡnɪm iːɭeː puroːɦɪtɐm",
  },
  iso: {
    supportsVedicAccents: true,
    kind: 'roman',
    "label": "ISO 15919",
    "keyboard": KEYBOARD_MAPS.iso,
  },
  itrans: {
    supportsVedicAccents: true,
    kind: 'roman',
    "label": "ITRANS",
    "sample": "shrI gurubhyo namaH . hariH OM",
    "keyboard": KEYBOARD_MAPS.itrans,
  },
  slp1: {
    supportsVedicAccents: false,
    kind: 'roman',
    "label": "SLP1",
    "keyboard": KEYBOARD_MAPS.slp1,
  },
  titus: {
    supportsVedicAccents: true,
    kind: 'roman',
    "label": "TITUS",
    "keyboard": KEYBOARD_MAPS.titus,
  },
  velthuis: {
    supportsVedicAccents: true,
    kind: 'roman',
    "label": "Velthuis",
    "keyboard": KEYBOARD_MAPS.velthuis,
  },
  wx: {
    supportsVedicAccents: false,
    kind: 'roman',
    "label": "WX",
    "keyboard": KEYBOARD_MAPS.wx,
  },
  separatorPersoArabic: {
    label: '── Perso-Arabic ──', separator: true,
  },
  urdu: {
    supportsVedicAccents: false,
    label: 'Urdu (Nastaliq)', kind: 'persoArabic', lang: 'ur', dir: 'rtl',
    fonts: ['Noto Nastaliq Urdu', 'Noto Naskh Arabic'],
    sample: 'اَ آ اِ اِی اُ اُو کَ کِ کُ کا کِی کُو',
    keyboard: KEYBOARD_MAPS.urdu,
  },
};


// Indices refer to the aligned engine tables above, preserving familiar key
// order and ASCII shortcuts. These are curated display choices; they do not
// restrict engine input. Tamil's palatal varga uses its superscripted forms.
const NATIVE_KEYBOARD_LAYOUTS = {
  assamese:  { vowels: '0-9 11-12 14-16', consonants: '0-27 29-32 41-42 44', otherDiacritics: '0-2', symbols: '0-13' },
  bengali:   { vowels: '0-9 11-12 14-16', consonants: '0-27 29-32 41-42 44', otherDiacritics: '0-2', symbols: '0-13' },
  burmese:   { vowels: '0-9 11-12 14-15', consonants: '0-33', otherDiacritics: '1-2', symbols: '0-9 11-12', extra: ['ျ', 'ြ', 'ွ', 'ှ', '္'] },
  devanagari:{ vowels: '0-18', consonants: '0-44', otherDiacritics: '0-2 4-5', symbols: '0-15' },
  grantha:   { vowels: '0-9 11-12 14-15', consonants: '0-33', otherDiacritics: '0-2', symbols: '0-15' },
  gujarati:  { vowels: '0-9 11-12 14-16 18', consonants: '0-33', otherDiacritics: '0-2', symbols: '0-13' },
  gurmukhi:  { vowels: '0-5 11-12 14-15', consonants: '0-29 31-33 38-41 43', otherDiacritics: '0-2 7', symbols: '0-12', extra: ['ੱ'] },
  kannada:   { vowels: '0-15', consonants: '0-35', otherDiacritics: '0-2 4-5', symbols: '0-15' },
  khmer:     { vowels: '0-9 11-12 14-16', vowelMarks: '0-5 11-12 14-16', consonants: '0-33', otherDiacritics: '1-2', symbols: '0-9 12-13', extra: ['្'] },
  malayalam: { vowels: '0-15 20', consonants: '0-36 52', otherDiacritics: '0-2', symbols: '0-15', extra: ['ൺ', 'ൻ', 'ർ', 'ൽ', 'ൾ', 'ൿ', 'ൔ', 'ൕ', 'ൖ', 'ൌ'] },
  oriya:     { vowels: '0-9 11-12 14-15', consonants: '0-33 41-42 44 54', otherDiacritics: '0-2', symbols: '0-13' },
  sinhala:   { vowels: '0-17', consonants: '0-33 43 45-49', otherDiacritics: '1-2', symbols: '0-9' },
  tamil:     { vowels: '0-5 10-15', consonants: '0-37', otherDiacritics: '3', symbols: '0-10' },
  telugu:    { vowels: '0-15', consonants: '0-35 50 63', otherDiacritics: '0-2 6', symbols: '0-15' },
  thai:      { vowels: '0-18', vowelMarks: '0-5 10-18', consonants: '0-33 38-40 43', otherDiacritics: '1-2', symbols: '0-9 11-12' },
  tibetan:   { vowels: '0-5 11-12 14-15', consonants: '0-7 9-32 37 40-42 50-51 53 63', otherDiacritics: '0-2 4-5', symbols: '0-13', extra: ['ྐ', 'ྑ', 'ྒ', 'ྒྷ', 'ྔ', 'ྕ', 'ྖ', 'ྗ', 'ྙ', 'ྚ', 'ྛ', 'ྜ', 'ྜྷ', 'ྞ', 'ྟ', 'ྠ', 'ྡ', 'ྡྷ', 'ྣ', 'ྤ', 'ྥ', 'ྦ', 'ྦྷ', 'ྨ', 'ྱ', 'ྲ', 'ླ', 'ྭ', 'ྴ', 'ྵ', 'ྶ', 'ྷ', 'ྯ', 'ྩ', 'ྪ', 'ྮ', 'ྫ', 'ྺ', 'ྻ', 'ྼ'] },
  urdu:      { vowels: '0-5 11-12 14-15', consonants: '0-8 10-13 15-29 31-32 37-43 53 55-62', otherDiacritics: '0', symbols: '0-9 11' }
};

function nativeKeyboardValues(script, group, values) {
  if (script === 'tamil' && group === 'consonants') {
    values = values.slice();
    values[7] = 'ச³';
    values[8] = 'ச⁴';
    values[37] = 'ஜ';
  }
  const layout = NATIVE_KEYBOARD_LAYOUTS[script];
  if (!layout) {
    // Roman transliteration schemes have no native alphabet. Keep their
    // distinct spellings, omitting repeated keys and duplicate vowel signs.
    if (group === 'vowelMarks') return values.map(() => '');
    const seen = new Set();
    return values.map(value => !value || seen.has(value) ? '' : (seen.add(value), value));
  }
  const indices = group === 'vowelMarks' ? (layout.vowelMarks || layout.vowels) : layout[group];
  if (!indices) return values;
  const selected = new Set();
  for (const part of indices.split(' ')) {
    const [first, last = first] = part.split('-').map(Number);
    for (let index = first; index <= last; index++) selected.add(index);
  }
  const seen = new Set();
  return values.map((value, index) => {
    if (!selected.has(index) || !value || seen.has(value)) return '';
    seen.add(value);
    return value;
  });
}

function nativeKeyboardExtras(script) {
  return (NATIVE_KEYBOARD_LAYOUTS[script]?.extra || []).map((value, index) => ({
    value, token: '.x' + index,
    consonant: script === 'burmese' && index < 4
  }));
}

const KEYBOARD_GROUPS = [
  { id: 'symbols', label: 'Symbols', property: 'symbols', color: 'number' },
  { id: 'otherDiacritics', label: 'Diacritics', property: 'otherDiacritics', color: 'diacritic' },
  { id: 'accents', label: 'Accents', property: 'accents', color: 'accent' },
  { id: 'vowels', label: 'Vowels', property: 'vowels', color: 'vowel' },
  { id: 'vowelMarks', label: 'Signs', property: 'vowelMarks', color: 'vowelmark' },
  { id: 'consonants', label: 'Consonants', property: 'consonants', start: 0, end: 34 },
  { id: 'additionalConsonants', label: 'Extended', property: 'consonants', start: 34, end: 64 },
  { id: 'nativeSigns', label: 'Script keys', property: 'nativeSigns', color: 'vowelmark' }
];

const CONSONANT_CATEGORIES = [
  { end: 5, color: 'varga-1', label: 'Ka-varga (Velar)' },
  { end: 10, color: 'varga-2', label: 'Cha-varga (Palatal)' },
  { end: 15, color: 'varga-3', label: 'Retroflex Ta-varga' },
  { end: 20, color: 'varga-4', label: 'Dental Ta-varga' },
  { end: 25, color: 'varga-5', label: 'Pa-varga (Labial)' },
  { end: 34, color: 'semivowel', label: 'Antastha/Ushman' },
  { end: 45, color: 'zha-yya', label: 'ऴ-य़ Group' },
  { end: 50, color: 'prenasal', label: 'Prenasalized Consonant' },
  { end: Infinity, color: 'additional', label: 'Additional consonants' }
];
