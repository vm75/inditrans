import { AltAccents, ScriptType, VedicAccents } from '../ts/src/type_defs';
import fs from 'fs';
import path from 'path';

const Tamil = ['tamilextended', 'granthagrantamil', 'tamil', 'tamilbrahmi', 'tamilgrantha'];
const NonIndic = ['oldpersian', 'hebrew'];
const Roman = ['romancolloquial', 'romansimple', 'isopali', 'romankana', 'barahanorth', 'barahasouth', 'mongolian', 'slp1', 'wancho', 'mro', 'iastpali', 'hanifirohingya', 'ariyaka', 'romanreadable', 'aksharaa', 'warangciti', 'sorasompeng', 'wx-kok', 'avestan', 'hk', 'iast', 'iso', 'itrans', 'titus', 'titus', 'velthuis', 'wx', 'inter', 'ipa', 'tolongsiki', 'santali', 'russiancyrillic'];

export function getScriptType(name: string) {
  name = name.toLowerCase();
  if (Roman.includes(name)) {
    return ScriptType.Roman;
  }
  if (NonIndic.includes(name)) {
    return ScriptType.Others;
  }
  if (Tamil.includes(name)) {
    return ScriptType.Tamil;
  }
  return ScriptType.Brahmi;
}

export class TranslitMap {
  // 'अ', 'आ', 'इ', 'ई', 'उ', 'ऊ', 'ऋ', 'ॠ', 'ऌ', 'ॡ', 'ऎ', 'ए', 'ऐ', 'ऒ', 'ओ', 'औ', 'ऍ', 'एॕ', 'ऑ'
  private v: string[] = [];   // vowels

  // (visarga) '्', 'ा', 'ि', 'ी', 'ु', 'ू', 'ृ', 'ॄ', 'ॢ', 'ॣ', 'ॆ', 'े', 'ै', 'ॊ', 'ो', 'ौ', 'ॅ', 'ॕ', 'ॉ'
  private vD: string[] = [];  // vowelDiacritics

  // 'क', 'ख', 'ग', 'घ', 'ङ'
  // 'च', 'छ', 'ज', 'झ', 'ञ'
  // 'ट', 'ठ', 'ड', 'ढ', 'ण'
  // 'त', 'थ', 'द', 'ध', 'न'
  // 'प', 'फ', 'ब', 'भ', 'म'
  // 'य', 'र', 'ल', 'व', 'श'
  // 'ष', 'स', 'ह',
  // 'ळ', ('ழ') 'ऴ', ('ற') 'ऱ', ('ன') 'ऩ',
  // (arabic) 'क़', 'ख़', 'ग़', 'ज़', 'ड़', 'ढ़', 'फ़', 'य़',
  // (sinhala) 'ँˆग', 'ँˆज', 'ँˆड', 'ँˆद', 'ँˆब'
  private c: string[] = [];   // consonants

  // 'ँ', 'ं', 'ः', (ayuda ezhuttu) 'ஃ', (nukta) '़'
  private cD: string[] = [];  // consonantDiacritics

  // '०', '१', '२', '३', '४', '५', '६', '७', '८', '९', 'ॐ', 'ऽ', '।', '॥', 'ꣳ' ('gͫ'), 'ꣴ' ('gͫ̄'), '\u200B', '\u200C', '\u200D'
  private s: string[] = [];   // symbols

  private t?: ScriptType; // script-type

  static vowels(obj: TranslitMap) {
    return obj.v;
  }

  static vowelDiacritics(obj: TranslitMap) {
    return obj.vD;
  }

  static consonants(obj: TranslitMap) {
    return obj.c;
  }

  static consonantDiacritics(obj: TranslitMap) {
    return obj.cD;
  }

  static symbols(obj: TranslitMap) {
    return obj.s;
  }

  static type(obj: TranslitMap) {
    return obj.t || ScriptType.Others;
  }

  setType(type: string) {
    switch (type) {
      case 'Brahmi':
        this.t = ScriptType.Brahmi;
        break;
      case 'Tamil':
        this.t = ScriptType.Tamil;
        break;
      case 'Roman':
        this.t = ScriptType.Roman;
        break;
      default:
        this.t = ScriptType.Others;
        break;
    }
  }
};

export function getAccents(name: string) {
  return getScriptType(name) === ScriptType.Roman ? AltAccents : VedicAccents;
}

export function getSpecialSymbols(name: string) {
  if (getScriptType(name) === ScriptType.Roman) {
    if (name === 'romanreadable') {
      return ['gum', 'ggum'];
    }
    return ['gͫ', 'gͫ̄'];

  }
  return ['ꣳ', 'ꣴ'];
}

export const ZeroWidthChars = ['\u200B', '\u200C', '\u200D'];
export function getZeroWidthChars(_name: string) {
  return ZeroWidthChars;
}

export const Diacritics = ['ʽ', '·', '¹', '²', '³', '⁴', '₁', '₂', '₃', '₄'];
export const DiacriticsRemovable = ['ʼ', 'ˇ', 'ˆ', '˘', '·'];

const dataDir = path.resolve('./data');

export class SerializableMap<K, V> extends Map<K, V> {
  toJSON() {
    return [...this.entries()];
  }
}

class MapLoader<T> {
  filePath: string;
  map: SerializableMap<string, T>;

  constructor(file: string) {
    this.filePath = path.join(dataDir, file);

    try {
      const json = JSON.parse(fs.readFileSync(this.filePath, 'utf8'));
      if (Array.isArray(json)) {
        this.map = new SerializableMap<string, T>(json);
      } else {
        this.map = new SerializableMap<string, T>(Object.entries(json));
      }
    } catch (error) {
      this.map = new SerializableMap<string, T>();
    }
  }

  entries() {
    return this.map.entries();
  }

  getSupportedScripts() {
    return [...this.map.keys()];
  }

  getScriptMap(name: string) {
    return this.map.get(name);
  }
}

// eslint-disable-next-line @typescript-eslint/no-explicit-any
type Any = any;

export type SanscriptMap = {
  isRomanScheme?: boolean
  vowels: Any,
  consonants: Any,
  vowel_marks?: Any,
  yogavaahas?: Any,
  virama: Any,
  symbols: Any,
  accents?: Any,
  extra_consonants?: Any,
  alternates?: Any,
  accented_vowel_alternates?: Any,
  zwj?: Any,
  zwnj?: Any,
  skip?: Any,
  candra?: Any
}

type AksharamukhaMap = {
  combiningsigns: {
    ayogavaha: string[],
    nukta: string[],
  },
  consonants: {
    main: string[],
    persoarabic: string[],
    sinhala: string[],
    south: string[],
  },
  numerals: string[],
  others: {
    aytham: string[],
    om: string[],
    symbols: string[],
  },
  vowels: {
    main: string[],
    modern: string[],
    sinhala: string[],
    south: string[],
  },
  vowelsigns: {
    main: string[],
    modern: string[],
    sinhala: string[],
    south: string[],
    virama: string[],
  }
};

function copy(toArray: string[], fromArray: string[], fromIndex: number, count = 1) {
  for (let i = fromIndex; i < fromIndex + count; i++) {
    const val = fromArray[i];
    toArray.push(val);
  }
}

function populateTranslitMap(name: string, aksharamukhaMap: AksharamukhaMap) {
  const map = new TranslitMap();

  copy(TranslitMap.vowels(map), aksharamukhaMap.vowels.main, 0, 10);
  copy(TranslitMap.vowels(map), aksharamukhaMap.vowels.south, 0);
  copy(TranslitMap.vowels(map), aksharamukhaMap.vowels.main, 10, 2);
  copy(TranslitMap.vowels(map), aksharamukhaMap.vowels.south, 1);
  copy(TranslitMap.vowels(map), aksharamukhaMap.vowels.main, 12, 2);
  copy(TranslitMap.vowels(map), aksharamukhaMap.vowels.modern, 0);
  copy(TranslitMap.vowels(map), aksharamukhaMap.vowels.sinhala, 0);
  copy(TranslitMap.vowels(map), aksharamukhaMap.vowels.modern, 1);
  copy(TranslitMap.vowelDiacritics(map), aksharamukhaMap.vowelsigns.virama, 0);
  copy(TranslitMap.vowelDiacritics(map), aksharamukhaMap.vowelsigns.main, 0, 9);
  copy(TranslitMap.vowelDiacritics(map), aksharamukhaMap.vowelsigns.south, 0);
  copy(TranslitMap.vowelDiacritics(map), aksharamukhaMap.vowelsigns.main, 9, 2);
  copy(TranslitMap.vowelDiacritics(map), aksharamukhaMap.vowelsigns.south, 1);
  copy(TranslitMap.vowelDiacritics(map), aksharamukhaMap.vowelsigns.main, 11, 2);
  copy(TranslitMap.vowelDiacritics(map), aksharamukhaMap.vowelsigns.modern, 0);
  copy(TranslitMap.vowelDiacritics(map), aksharamukhaMap.vowelsigns.sinhala, 0);
  copy(TranslitMap.vowelDiacritics(map), aksharamukhaMap.vowelsigns.modern, 1);
  copy(TranslitMap.consonants(map), aksharamukhaMap.consonants.main, 0, 33);
  copy(TranslitMap.consonants(map), aksharamukhaMap.consonants.south, 0, 4);
  copy(TranslitMap.consonants(map), aksharamukhaMap.consonants.persoarabic, 0, 8);
  copy(TranslitMap.consonants(map), aksharamukhaMap.consonants.sinhala, 0, 5);
  copy(TranslitMap.consonantDiacritics(map), aksharamukhaMap.combiningsigns.ayogavaha, 0, 3);
  copy(TranslitMap.consonantDiacritics(map), aksharamukhaMap.others.aytham, 0);
  copy(TranslitMap.consonantDiacritics(map), aksharamukhaMap.combiningsigns.nukta, 0);
  copy(TranslitMap.symbols(map), aksharamukhaMap.numerals, 0, 10);
  copy(TranslitMap.symbols(map), aksharamukhaMap.others.om, 0);
  copy(TranslitMap.symbols(map), aksharamukhaMap.others.symbols, 0, 3);
  const symbols = getSpecialSymbols(name);
  copy(TranslitMap.symbols(map), symbols, 0, symbols.length);
  const zeroWidthChars = getZeroWidthChars(name);
  copy(TranslitMap.symbols(map), zeroWidthChars, 0, zeroWidthChars.length);

  return map;
}

export function generateTranslitMaps() {
  const maps = new SerializableMap();
  const aksharamukha = new MapLoader<AksharamukhaMap>('aksharamukha-map.json');

  for (const [name, aksharamukhaMap] of aksharamukha.entries()) {
    maps.set(name, populateTranslitMap(name, aksharamukhaMap));
  }

  fs.writeFileSync(path.join(dataDir, 'scripts-gen.json'), JSON.stringify(maps, undefined, 2), 'utf8');
}

generateTranslitMaps();
