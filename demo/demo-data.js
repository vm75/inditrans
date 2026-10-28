// Demo configuration used by index.html.
const ROMAN_FONTS = ["Inter", "Georgia", "Gentium Plus", "Andika", "Charis SIL (installed)", "system-ui", "serif", "sans-serif"];
// External font services use the subset name 'latin' for Roman fonts.
const SCRIPT_FONT_PROBES = {
  devanagari: 'कमा', tamil: 'கமா', telugu: 'కమా', kannada: 'ಕಮಾ',
  malayalam: 'കമാ', grantha: '𑌕𑌮𑌾', bengali: 'কমা', gujarati: 'કમા', gurmukhi: 'ਕਮਾ',
  assamese: 'কমা', burmese: 'ကမာ', khmer: 'កមា', sinhala: 'කමා',
  oriya: 'କମା', thai: 'กมา', tibetan: 'ཀམ', arabic: 'کَما', latin: 'Aa'
};

// Add hosted families here, then list them in a script's fonts (or ROMAN_FONTS).
// Unlisted fonts use an installed family; the custom-font prompt remains available.
const FONT_RESOURCES = {
  "Noto Nastaliq Urdu": "wght@400;700",
  "Noto Naskh Arabic": "wght@400;700",
  "Andika": "wght@400;700",
  "Gentium Plus": "wght@400;700",
  "Inter": "wght@400;700",
  "Noto Sans Bengali": "wght@400;700",
  "Noto Sans Devanagari": "wght@400;700",
  "Noto Sans Grantha": "",
  "Noto Sans Gujarati": "wght@400;700",
  "Noto Sans Gurmukhi": "wght@400;700",
  "Noto Sans Kannada": "wght@400;700",
  "Noto Sans Malayalam": "wght@400;700",
  "Noto Sans Oriya": "wght@400;700",
  "Noto Sans Sinhala": "wght@400;700",
  "Noto Sans Tamil": "wght@400;700",
  "Noto Sans Telugu": "wght@400;700",
  "Noto Serif Bengali": "wght@400;700",
  "Noto Serif Devanagari": "wght@400;700",
  "Noto Serif Gujarati": "wght@400;700",
  "Noto Serif Gurmukhi": "wght@400;700",
  "Noto Serif Kannada": "wght@400;700",
  "Noto Serif Malayalam": "wght@400;700",
  "Noto Serif Oriya": "wght@400;700",
  "Noto Serif Sinhala": "wght@400;700",
  "Noto Serif Tamil": "wght@400;700",
  "Noto Serif Telugu": "wght@400;700",
  "Tiro Bangla": "",
  "Tiro Devanagari Sanskrit": "",
  "Tiro Kannada": "",
  "Tiro Tamil": "",
  "Tiro Telugu": ""
};

const OPTION_DEFINITIONS = [
  { name: 'TamilTraditional', label: 'Traditional Tamil', tamilOnly: true, description: 'Use traditional Tamil consonants' },
  { name: 'TamilSuperscripted', label: 'Superscripted Tamil', tamilOnly: true, description: 'Use superscripted Tamil consonants' },
  { name: 'ASCIINumerals', label: 'ASCII numerals', description: 'Use 0–9 for numbers in the output' },
  { name: 'IgnoreVedicAccents', label: 'Ignore Vedic accents', description: 'Remove Vedic accents during conversion' },
  { name: 'RetainSpecialMarkers', label: 'Retain special markers', description: 'Keep markers used for non-standard characters' },
  { name: 'NoXMLTagHandling', label: 'Treat HTML/XML as text', description: 'Transliterate inside markup instead of preserving tags' },
  { name: 'DisableInference', label: 'Disable inference', description: 'Disable contextual Tamil pronunciation and anusvara assimilation rules' }
];

const DEMO_DEFAULTS = { sourceScript: 'indic', targetScript: 'indlish', skipStart: '##', skipEnd: '##' };

// Text restored by the demo reset control.
const DEMO_SAMPLE = `<अஅa-xml-tag>
१ - श्री॒ गु॒रु॒भ्यो नमः॒ । ॐ ग॒णानां᳚ त्वा ग॒णप॑तिꣳ हवामहे ॥
२ - संगीत और संस्कृत — अनेक भाषाएँ, एक संवाद।
৩ - কষ্ট না করলে কেষ্ট মেলে না।
૪ - ધીરજના ફળ મીઠા।
੫ - ਅੰਤ ਭਲੇ ਦਾ ਭਲਾ।
௬ - ப⁴க³வான் அருளால் உழவனின் களஞ்சியம் நிறையும்।
೭ - ಕೈ ಕೆಸರಾದರೆ ಬಾಯಿ ಮೊಸರು।
౮ - దీపం ఉన్నప్పుడే ఇల్లు చక్కదిద్దుకోవాలి।
൯ - മിന്നുന്നതെല്ലാം പൊന്നല്ല।
##অসমীয়া বাংলা မြန်မာ देवनागरी 𑌗𑍍𑌰𑌨𑍍𑌥 ગુજરાતી ਗੁਰਮੁਖੀ ಕನ್ನಡ ខ្មែរ മലയാളം ଓଡ଼ିଆ සිංහල தமிழ் తెలుగు ไทย བོད་ཡིག##
</अஅa-xml-tag>`;
