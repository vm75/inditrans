export enum ScriptType {
  Tamil,
  Indic,
  Latin,
  Others,
}

export function getScriptTypeFromString(name: string) {
  switch (name) {
    case 'Tamil':
      return ScriptType.Tamil;
    case 'Indic':
      return ScriptType.Indic;
    case 'Latin':
      return ScriptType.Latin;
    default:
      return ScriptType.Others;
  }
}

export enum TokenType {
  Vowel = 0,
  VowelDiacritic = 1,
  Consonant = 2,
  ConsonantDiacritic = 3,
  Symbol = 4,
  Accent = 5,
  ToggleTrans = 6,
  Ignore = 7,
}

export function tokenTypeStr(type: TokenType) {
  const str = [
    'Vowel',
    'VowelDiacritic',
    'Consonant',
    'ConsonantDiacritic',
    'Symbol',
    'Accent',
    'ToggleTrans',
    'Ignore',
  ];
  return str[type];
}

export enum SpecialIndices {
  Virama = 0,
  Anunasika = 0,
  Anuswara = 1,
  Visarga = 2,
  AyudaEzhuttu = 3,
  Nukta = 4,
  Om = 10,
  ZeroWidthSpace = 16,
  ZeroWidthNonJoiner = 17,
  ZeroWidthJoiner = 18,
  क = 0,
  ख = 1,
  ग = 2,
  घ = 3,
  ङ = 4,
  च = 5,
  छ = 6,
  ज = 7,
  झ = 8,
  ञ = 9,
  ट = 10,
  ठ = 11,
  ड = 12,
  ढ = 13,
  ण = 14,
  त = 15,
  थ = 16,
  द = 17,
  ध = 18,
  न = 19,
  प = 20,
  फ = 21,
  ब = 22,
  भ = 23,
  म = 24,
  य = 25,
  र = 26,
  ल = 27,
  व = 28,
  श = 29,
  ष = 30,
  स = 31,
  ह = 32,
  ळ = 33,
  ழ = 34,
  ற = 35,
  ன = 36,
  क़ = 37,
  ख़ = 38,
  ग़ = 39,
  ज़ = 40,
  ड़ = 41,
  ढ़ = 42,
  फ़ = 43,
  य़ = 44,
  ඟ = 45,
  ඦ = 46,
  ඬ = 47,
  ඳ = 48,
  ඹ = 49,
  க = 0,
  ச = 5,
  ஜ = 7,
  ட = 10,
  த = 15,
  ந = 19,
  ப = 20,
  ஸ = 31,
}

export type AliasEntry = {
  tokenType: TokenType;
  scriptType: ScriptType;
  idx: number;
  alts: string[];
};

export const PositionalAliases: AliasEntry[] = [
  {
    tokenType: TokenType.Consonant,
    scriptType: ScriptType.Indic,
    idx: 37 /* क़ */,
    alts: ['क़'],
  },
  {
    tokenType: TokenType.Consonant,
    scriptType: ScriptType.Indic,
    idx: 38 /* ख़ */,
    alts: ['ख़'],
  },
  {
    tokenType: TokenType.Consonant,
    scriptType: ScriptType.Indic,
    idx: 39 /* ग़ */,
    alts: ['ग़'],
  },
  {
    tokenType: TokenType.Consonant,
    scriptType: ScriptType.Indic,
    idx: 40 /* ज़ */,
    alts: ['ज़'],
  },
  {
    tokenType: TokenType.Consonant,
    scriptType: ScriptType.Indic,
    idx: 41 /* ड़ */,
    alts: ['ड़'],
  },
  {
    tokenType: TokenType.Consonant,
    scriptType: ScriptType.Indic,
    idx: 42 /* ढ़ */,
    alts: ['ढ़'],
  },
  {
    tokenType: TokenType.Consonant,
    scriptType: ScriptType.Indic,
    idx: 43 /* फ़ */,
    alts: ['फ़'],
  },
  {
    tokenType: TokenType.Consonant,
    scriptType: ScriptType.Indic,
    idx: 44 /* य़ */,
    alts: ['य़'],
  },
  {
    tokenType: TokenType.VowelDiacritic,
    scriptType: ScriptType.Indic,
    idx: 0 /* ् */,
    alts: ['᳭', '്‍', '‍್', '්‍', '्‍'],
  },
  {
    tokenType: TokenType.VowelDiacritic,
    scriptType: ScriptType.Tamil,
    idx: 13 /* ொ */,
    alts: ['ொ'],
  },
  {
    tokenType: TokenType.VowelDiacritic,
    scriptType: ScriptType.Tamil,
    idx: 14 /* ோ */,
    alts: ['ோ'],
  },
  {
    tokenType: TokenType.VowelDiacritic,
    scriptType: ScriptType.Tamil,
    idx: 15 /* ௌ */,
    alts: ['ௌ'],
  },
  {
    tokenType: TokenType.Accent,
    scriptType: ScriptType.Indic,
    idx: 1 /*꠰  ॑ */,
    alts: ['◌॑'],
  },
  {
    tokenType: TokenType.Accent,
    scriptType: ScriptType.Latin,
    idx: 0 /*꠰  ॒ */,
    alts: ['̱', '↓', '\\_', '\\`', "'", '`'],
  },
  {
    tokenType: TokenType.Accent,
    scriptType: ScriptType.Latin,
    idx: 1 /*꠰  ॑ */,
    alts: ['̍', '↑', "\\'", '\\’', '̭'],
  },
  {
    tokenType: TokenType.Accent,
    scriptType: ScriptType.Latin,
    idx: 2 /*꠰  ᳚ */,
    alts: ['̎', '↑↑', '\\"', "\\''", '\\’’'],
  },
  {
    tokenType: TokenType.ConsonantDiacritic,
    scriptType: ScriptType.Tamil,
    idx: 0 /* ँ */,
    alts: ['ம்ˮ'],
  },
  {
    tokenType: TokenType.ConsonantDiacritic,
    scriptType: ScriptType.Tamil,
    idx: 1 /* ं */,
    alts: ['ம்ʼ'],
  },
  {
    tokenType: TokenType.ConsonantDiacritic,
    scriptType: ScriptType.Latin,
    idx: 0 /* ँ */,
    alts: ['~', 'm̐', 'ṁ'],
  },
  {
    tokenType: TokenType.ConsonantDiacritic,
    scriptType: ScriptType.Latin,
    idx: 1 /* ं */,
    alts: ['ṃ', 'ṃ'],
  },
  {
    tokenType: TokenType.ConsonantDiacritic,
    scriptType: ScriptType.Latin,
    idx: 2 /* ः */,
    alts: ['ḥ', 'ḥ'],
  },
  {
    tokenType: TokenType.Symbol,
    scriptType: ScriptType.Indic,
    idx: 10 /* ॐ */,
    alts: ['ओ३म्'],
  },
  {
    tokenType: TokenType.Symbol,
    scriptType: ScriptType.Latin,
    idx: 10 /* ॐ */,
    alts: ['o3m', 'OM', 'AUM', 'oṃ', 'ŏṃ'],
  },
  {
    tokenType: TokenType.Symbol,
    scriptType: ScriptType.Latin,
    idx: 12 /* ꠰ */,
    alts: ['|', '.'],
  },
  {
    tokenType: TokenType.Symbol,
    scriptType: ScriptType.Latin,
    idx: 13 /* ॥ */,
    alts: ['||', '꠰꠰', '..'],
  },
  {
    tokenType: TokenType.Symbol,
    scriptType: ScriptType.Latin,
    idx: 14 /* ꣳ */,
    alts: ['gͫ', '\\m+', '{\\m+}', '\\м+'],
  },
  {
    tokenType: TokenType.Symbol,
    scriptType: ScriptType.Latin,
    idx: 15 /* ꣴ */,
    alts: ['gͫ̄', '\\m++', '\\м++'],
  },
  {
    tokenType: TokenType.Symbol,
    scriptType: ScriptType.Latin,
    idx: 17 /* \u200C */,
    alts: ['{}', '^^'],
  },
  {
    tokenType: TokenType.Symbol,
    scriptType: ScriptType.Latin,
    idx: 18 /* \u200D */,
    alts: ['()', '^'],
  },
];

/*
  'alternates': { 'm̐': ['ṁ'], 'ṃ': ['ṃ'], 'ḥ': ['ḥ'], 'ṭ': ['ṭ'], 'ṭh': ['ṭh'], 'ḍ': ['ḍ'], 'ḍh': ['ḍh'], 'ṇ': ['ṇ'], 'ṝ': ['ṝ', 'r̥̄', 'r̥̄', 'ṝ', 'ṝ'], 'ṅ': ['ṅ'], 'ñ': ['ñ'], 'ṣ': ['ṣ'], 'ṛ': ['r̥', 'ṛ'], 'ṛh': ['ṛh'], '́': ['¹'] },
  'alternates': { 'm̐': ['ṁ'], 'ṃ': ['ṃ'], 'ḥ': ['ḥ'], 'ṭ': ['ṭ'], 'ṭh': ['ṭh'], 'ḍ': ['ḍ'], 'ḍh': ['ḍh'], 'ṇ': ['ṇ'], 'ṝ': ['ṝ', 'r̥̄', 'r̥̄', 'ṝ', 'ṝ'], 'ṅ': ['ṅ'], 'ñ': ['ñ'], 'ṣ': ['ṣ'], 'ṛ': ['ṛ'], 'ṛh': ['ṛh'], '́': ['¹'] },
  'alternates': { 'm̐': ['ṁ'], 'ṃ': ['ṃ'], 'ḥ': ['ḥ'], 'ṭ': ['ṭ'], 'ṭh': ['ṭh'], 'ḍ': ['ḍ'], 'ḍh': ['ḍh'], 'ṇ': ['ṇ'], 'ṝ': ['ṝ', 'r̥̄', 'r̥̄', 'ṝ', 'ṝ'], 'ṅ': ['ṅ'], 'ñ': ['ñ'], 'ṣ': ['ṣ'], 'ṛ': ['r̥', 'ṛ'], '́': ['¹'] },
  'alternates': { '|': ['.'], '||': ['..'], 'A': ['aa'], 'I': ['ee'], 'U': ['oo'], 'kh': ['K'], 'gh': ['G'], 'ch': ['c'], 'Ch': ['C'], 'jh': ['J'], 'ph': ['P'], 'bh': ['B'], '~n': ['JN'], 'v': ['w'], 'sh': ['S'], 'h': ['~h'], 'V': ['x'] },
  'alternates': { '|': ['.'], '||': ['..'], 'A': ['aa'], 'I': ['ii', 'ee'], 'U': ['uu', 'oo'], 'RRi': ['R^i'], 'RRI': ['R^I'], 'LLi': ['L^i'], 'LLI': ['L^I'], 'M': ['.m', '.n'], '~N': ['N^'], 'ch': ['c'], 'Ch': ['C', 'chh'], '~n': ['JN'], 'v': ['w'], 'Sh': ['S', 'shh'], 'kSh': ['kS', 'x'], 'j~n': ['GY', 'dny'], 'OM': ['AUM'], '\\_': ['\\`'], '\\_H': ['\\`H'], "\\'M": ["\\'.m", "\\'.n"], '\\_M': ['\\_.m', '\\_.n', '\\`M', '\\`.m', '\\`.n'], '.a': ['~'], 'z': ['J'] },
  'alternates': { '|': ['.'], '||': ['..'], 'A': ['aa'], 'I': ['ii', 'ee'], 'U': ['uu'], 'R': ['RRi', 'R^i'], 'RR': ['RRI', 'R^I'], 'LLi': ['L^i'], 'LLI': ['L^I'], 'M': ['.m', '.n'], 'ch': ['c'], 'Ch': ['C'], 'jh': ['J'], 'ph': ['P'], 'bh': ['B'], '~n': ['JN'], 'v': ['w'], 'Sh': ['S'], 'x': ['kS', 'ksh'], 'jn': ['GY', 'dny'], 'z': ['J'] },
  'alternates': { 'A': ['aa'], 'I': ['ii', 'ee'], 'U': ['uu', 'oo'], 'RRi': ['R^i'], 'RRI': ['R^I'], 'LLi': ['L^i'], 'LLI': ['L^I'], 'M': ['.m', '.n'], '~N': ['N^'], 'ch': ['c'], 'Ch': ['C', 'chh'], '~n': ['JN'], 'v': ['w'], 'Sh': ['S', 'shh'], 'kSh': ['kS', 'x'], 'j~n': ['GY', 'dny'], 'OM': ['AUM'], '\\_': ['\\`'], '\\_H': ['\\`H'], "\\'M": ["\\'.m", "\\'.n"], '\\_M': ['\\_.m', '\\_.n', '\\`M', '\\`.m', '\\`.n'], '.a': ['~'], '|': ['.'], '||': ['..'], 'z': ['J'] },
  'alternates': { 'ಜ಼': ['ಸ಼'] }
  'alternates': { 'm̐': ['ṁ'], 'oṃ': ['ŏṃ'], 'r̥̄': ['r̥̄'], '.': ['|', '/'], '..': ['||', '//'] },

  'accented_vowel_alternates': { 'á': ['á'], 'é': ['é'], 'í': ['í'], 'ó': ['ó'], 'ú': ['ú'], 'ŕ̥': ['ŕ̥'], 'à': ['à'], 'è': ['è'], 'ì': ['ì'], 'ò': ['ò'], 'ù': ['ù'], 'r̥̀': ['r̥̀'] },
  'accented_vowel_alternates': { 'á': ['á'], 'é': ['é'], 'í': ['í'], 'ó': ['ó'], 'ú': ['ú'], 'ṛ́': ['ŕ̥'], 'à': ['à'], 'è': ['è'], 'ì': ['ì'], 'ò': ['ò'], 'ù': ['ù'], 'ṛ̀': ['r̥̀', 'ṛ̀'] },

  'accents': { '॑': '॑', '॒': '॒', '᳡': '᳡', '꣡': '꣡', '꣢': '꣢', '꣣': '꣣', '꣤': '꣤', '꣥': '꣥', '꣦': '꣦', '꣧': '꣧', '꣨': '꣨', '꣩': '꣩', '꣪': '꣪', '꣫': '꣫', '꣬': '꣬', '꣭': '꣭', '꣮': '꣮', '꣯': '꣯', '꣰': '꣰', '꣱': '꣱' },
  'accents': { '॑': '॑', '॒': '॒' },
  'accents': { '॑': "\\'", '॒': '\\_' },
  'accents': { '॑': '#', '॒': 'q', '᳡': 'Q', '꣡': 'V', '꣢': 'W', '꣣': '$' },

  'alternates': { '|': ['.', '/'], '||': ['..', '//'], "'": ['`'], 'm̐': ['ṁ'], 'ṃ': ['ṃ'], 'ḥ': ['ḥ'], 'ṭ': ['ṭ'], 'ṭh': ['ṭh'], 'ḍ': ['ḍ'], 'ḍh': ['ḍh'], 'ṇ': ['ṇ'], 'ṣ': ['ṣ'], 'ṅ': ['ṅ'], 'ñ': ['ñ'], 'ṛ': ['r̥', 'ṛ'], 'ṝ': ['ṝ', 'r̥̄', 'r̥̄', 'ṝ', 'ṝ'], '́': ['¹'] },
  'alternates': { '|': ['.', '/'], '||': ['..', '//'], "'": ['`'], 'm̐': ['ṁ'], 'ṝ': ['ṝ', 'r̥̄', 'r̥̄', 'ṝ', 'ṝ'], 'ṃ': ['ṃ'], 'ḥ': ['ḥ'], 'ṭ': ['ṭ'], 'ṭh': ['ṭh'], 'ḍ': ['ḍ'], 'ḍh': ['ḍh'], 'ṇ': ['ṇ'], 'ṅ': ['ṅ'], 'ñ': ['ñ'], 'ṣ': ['ṣ'], 'ṛ': ['ṛ'], 'ṛh': ['ṛh'], '́': ['¹'] },
  'alternates': { '|': ['.', '/'], '||': ['..', '//'], "'": ['`'], 'm̐': ['ṁ'], 'ṃ': ['ṃ'], 'ḥ': ['ḥ'], 'ṭ': ['ṭ'], 'ṭh': ['ṭh'], 'ḍ': ['ḍ'], 'ḍh': ['ḍh'], 'ṇ': ['ṇ'], 'ṅ': ['ṅ'], 'ñ': ['ñ'], 'ṣ': ['ṣ'], 'ṛ': ['r̥', 'ṛ'], 'ṝ': ['ṝ', 'r̥̄', 'r̥̄', 'ṝ', 'ṝ'], '́': ['¹'] },


Alternates:
//꠰ '॒'   : '̱',     '↓',      '\\_'      '\\`'
//꠰ '॑'   : '̍',     '↑',      "\\'"                '\\’'
//꠰ '᳚'   : '̎',     '↑↑',     '\\"'      "\\''"    '\\’’'
//꠰ 'ꣳ'   : 'gͫ'     '\\m+'    '{\\m+}'  '\\м+'     'г\u035Cм'  gum   '𑍞' (grantha)
//꠰ 'ꣴ'   : 'gͫ̄'     '\\m++'             '\\м++'    'г\u0361м'  ggum  '𑍟' (grantha)
ZWS  '\u200B'
ZWNJ '{}', '\u200C' '^^' (baraha)
ZWJ  '()', '\u200D' '^'  (baraha)
//꠰ '𑆃᳘' '𑆇ʼ' (sharada)
//  '꠰꠰' '॥'
// '।।','॥'
//  '।', '꠰'

*/

// Injected only if already supported by script
export const DependantAliases = {
  'ਂ': ['ੰ'],
  // '́': ['¹'],
  // 'A': ['aa'], 'I': ['ii', 'ee'], 'U': ['uu', 'oo'], 'RRi': ['R^i'], 'RRI': ['R^I'], 'LLi': ['L^i'], 'LLI': ['L^I'], 'M': ['.m', '.n'], '~N': ['N^'], 'ch': ['c'], 'Ch': ['C', 'chh'], '~n': ['JN'], 'v': ['w'], 'Sh': ['S', 'shh'], 'kSh': ['kS', 'x'], 'j~n': ['GY', 'dny'],
  // '\\_H': ['\\`H'], "\\'M": ["\\'.m", "\\'.n"], '\\_M': ['\\_.m', '\\_.n', '\\`M', '\\`.m', '\\`.n'],
  // 'ṭ': ['ṭ'], 'ṭh': ['ṭh'], 'ḍ': ['ḍ'], 'ḍh': ['ḍh'], 'ṇ': ['ṇ'], 'ṝ': ['ṝ', 'r̥̄', 'r̥̄', 'ṝ', 'ṝ'], 'ṅ': ['ṅ'], 'ñ': ['ñ'], 'ṣ': ['ṣ'], 'ṛ': ['r̥', 'ṛ'], 'ṛh': ['ṛh'],
  // 'á': ['á'], 'é': ['é'], 'í': ['í'], 'ó': ['ó'], 'ú': ['ú'], 'à': ['à'], 'è': ['è'], 'ì': ['ì'], 'ò': ['ò'], 'ù': ['ù'], 'ṛ́': ['ŕ̥'], 'ṛ̀': ['r̥̀', 'ṛ̀'],
};

export const VedicAccents = [
  /*꠰*/ '॒',
  '॑',
  '᳚',
  '᳡',
  '꣡',
  '꣢',
  '꣣',
  '꣤',
  '꣥',
  '꣦',
  '꣧',
  '꣨',
  '꣩',
  '꣪',
  '꣫',
  '꣬',
  '꣭',
  '꣮',
  '꣯',
  '꣰',
  '꣱',
];
export const BasicAccents = [
  /*꠰*/ '॒',
  '॑',
  '᳚',
  '',
  '',
  '',
  '',
  '',
  '',
  '',
  '',
  '',
  '',
  '',
  '',
  '',
  '',
  '',
  '',
  '',
  '',
];
export const AltAccents = [
  /*꠰*/ '̱',
  '̍',
  '̎',
  '',
  '',
  '',
  '',
  '',
  '',
  '',
  '',
  '',
  '',
  '',
  '',
  '',
  '',
  '',
  '',
  '',
  '',
];

export const TamilSuperscripts = '¹²³⁴';
export const TamilSpecialChars = 'ʼˮˇ꞉ஃ·';
export const QuotedMarkers = 'ʼˮ';
