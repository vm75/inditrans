import * as TranslitTypes from './type_defs';
import TranslitData from '../../assets/scripts.json' assert { type: 'json' };
import { TranslitOptions } from './translit_options';
import { Trie } from './trie';

let errorLogger: (text: string) => void;

export function setErrorLogger(func: (text: string) => void) {
  errorLogger = func;
}

function getTranslitOptions(opts: string): TranslitOptions {
  const options: TranslitOptions = {};
  opts.split(/\s+/).forEach(opt => {
    switch (opt) {
      case 'TamilTraditional':
        options.TamilTraditional = true;
        break;
      case 'TamilSuperscripted':
        options.TamilSuperscripted = true;
        break;
      case 'IgnoreQuotedMarkers':
        options.IgnoreQuotedMarkers = true;
        break;
      case 'IgnoreVedicAccents':
        options.IgnoreVedicAccents = true;
        break;
      case 'InferAnuswara':
        options.InferAnuswara = true;
        break;
      case 'RetainZeroWidthChars':
        options.RetainZeroWidthChars = true;
        break;
      case 'ASCIINumerals':
        options.ASCIINumerals = true;
        break;
    }
  })
  return options;
}

function scriptIsReadable(name: string): boolean {
  const notReadable = ['easyroman', 'romanreadable', 'romancolloquial'];
  return notReadable.indexOf(name) < 0;
}

type Token = {
  readonly tokenType: TranslitTypes.TokenType,
  readonly scriptType: TranslitTypes.ScriptType,
  readonly idx: number
};

type ScriptData = {
  type: TranslitTypes.ScriptType;
  charMaps: string[][];
};

class ScriptReaderMap {
  name: string;
  type: TranslitTypes.ScriptType;
  tokenMap = new Trie<Token>();

  addCharMap(script: string, tokenType: TranslitTypes.TokenType, scriptType: TranslitTypes.ScriptType, map: string[], count: number) {
    for (let idx = 0; idx < count; idx++) {
      if (scriptType == TranslitTypes.ScriptType.Roman && tokenType == TranslitTypes.TokenType.VowelDiacritic && idx > TranslitTypes.SpecialIndices.Virama) {
        continue;
      }
      const res = this.tokenMap.addLookup(map[idx], { scriptType, tokenType, idx });
      if (res != undefined && errorLogger != undefined) {
        const error = `Error adding for: ${script}, tokenType: ${TranslitTypes.tokenTypeStr(tokenType)}, idx: ${idx.toString()}, new value: ${map[idx]}`;
        errorLogger(error);
      }
    }
  }

  constructor(name: string, scriptData: ScriptData) {
    this.name = name;
    this.type = scriptData.type;

    const scriptType = scriptData.type;

    this.addScript(name, scriptData);
    this.addCharMap(name, TranslitTypes.TokenType.Accent, scriptType, TranslitTypes.VedicAccents, TranslitTypes.VedicAccents.length);

    this.tokenMap.addLookup('##', { tokenType: TranslitTypes.TokenType.ToggleTrans, scriptType: scriptData.type, idx: 0 });

    TranslitTypes.PositionalAliases.forEach(aliasEntry => {
      if (aliasEntry.scriptType != scriptType) {
        return;
      }
      aliasEntry.alts.forEach(alt => {
        this.tokenMap.addLookup(alt, { tokenType: aliasEntry.tokenType, scriptType: aliasEntry.scriptType, idx: aliasEntry.idx });
      });
    });
  }

  addScript(name: string, scriptData: ScriptData) {
    for (let tokenType = TranslitTypes.TokenType.Vowel; tokenType <= TranslitTypes.TokenType.Symbol; tokenType++) {
      this.addCharMap(name, tokenType, scriptData.type, scriptData.charMaps[tokenType], scriptData.charMaps[tokenType].length);
    }
  }

  lookupToken(text: string, startIndex: number): { match: Token | undefined, len: number } {
    return this.tokenMap.lookup(text, startIndex);
  }

  findNextToken(text: string, startIndex: number): { match: Token | undefined, len: number, index: number } {
    let index = startIndex;
    let result;
    while ((result = this.tokenMap.lookup(text, index)).match == undefined) {
      if (index == text.length) {
        return { match: undefined, len: 0, index: index };
      }
      index++;
    }

    return { match: result.match, len: result.len, index };
  }
}

class ScriptWriterMap {
  name: string;
  type: TranslitTypes.ScriptType;
  charMaps: string[][];

  constructor(name: string, scriptData: ScriptData) {
    this.name = name;
    this.type = scriptData.type;
    this.charMaps = [...scriptData.charMaps];
    const accents = this.type == TranslitTypes.ScriptType.Roman ? TranslitTypes.AltAccents : TranslitTypes.VedicAccents;
    this.charMaps.push(accents);
  }

  lookupChar(type: TranslitTypes.TokenType, idx: number): string { return this.charMaps[type][idx]; }
  lookup(token: Token): string { return this.charMaps[token.tokenType][token.idx]; }
}

type TokenOrString = Token | string;

type TokenUnit = {
  leadToken: Token,
  vowelDiacritic?: Token,
  consonantDiacritic?: Token,
  accent?: Token,
}

type TokenUnitOrString = TokenUnit | string;

class InputReader {
  options: TranslitOptions;
  wordStart = true;
  tokenUnits: TokenOrString[];
  numTokenUnits = 0;
  tokenUnitIndex = 0;

  constructor(text: string, map: ScriptReaderMap, options: TranslitOptions) {
    this.options = options;
    this.tokenUnits = new Array(text.length);

    let skipTrans = false;
    let index = 0;
    const end = text.length;
    while (index < end) {
      if (skipTrans) {
        const start = index;
        while (text.charAt(index) != '#' && text.charAt(index + 1) != '#' && index < end) {
          index++;
        }
        if (index > start) {
          this.tokenUnits[this.numTokenUnits++] = text.substring(start, index);
          continue;
        }
      }

      const result = map.lookupToken(text, index);
      if (result.match != undefined) {
        if (result.match.tokenType == TranslitTypes.TokenType.ToggleTrans) {
          skipTrans = !skipTrans;
        } else {
          this.tokenUnits[this.numTokenUnits++] = result.match;
        }
        index += result.len;
      } else {
        const start = index++;
        while (index < end && map.lookupToken(text, index).match == undefined) {
          index++;
        }
        this.tokenUnits[this.numTokenUnits++] = text.substring(start, index);
      }
    }
  }

  hasMore() {
    return this.tokenUnitIndex < this.numTokenUnits;
  }

  getNext(): TokenUnitOrString {
    const next = this.tokenUnits[this.tokenUnitIndex++];
    if (typeof next == 'string') {
      this.wordStart = true;
      return next;
    }

    let tokenUnit: TokenUnit = { leadToken: next };
    switch (next.scriptType) {
      case TranslitTypes.ScriptType.Brahmi:
        tokenUnit = this.readBrahmiTokenUnit(next);
        break;
      case TranslitTypes.ScriptType.Tamil:
        tokenUnit = this.readTamilTokenUnit(next);
        break;
      case TranslitTypes.ScriptType.Roman:
        tokenUnit = this.readRomanTokenUnit(next);
        break;
      default:
        break;
    }
    this.wordStart = (tokenUnit.leadToken.tokenType == TranslitTypes.TokenType.Symbol || tokenUnit.leadToken.tokenType == TranslitTypes.TokenType.ToggleTrans);
    return tokenUnit;
  }

  readBrahmiTokenUnit(start: Token): TokenUnit {
    const tokens: TokenUnit = { leadToken: start };
    if (start.tokenType == TranslitTypes.TokenType.Consonant) {
      let nextToken;
      while (this.tokenUnitIndex < this.numTokenUnits && typeof (nextToken = this.tokenUnits[this.tokenUnitIndex]) !== 'string') {
        switch (nextToken.tokenType) {
          case TranslitTypes.TokenType.ConsonantDiacritic:
            tokens.consonantDiacritic = nextToken;
            break;
          case TranslitTypes.TokenType.VowelDiacritic:
            tokens.vowelDiacritic = nextToken;
            break;
          case TranslitTypes.TokenType.Accent:
            tokens.accent = nextToken;
            break;
          default:
            return tokens;
        }
        this.tokenUnitIndex++;
      }
    } else if (start.tokenType == TranslitTypes.TokenType.Vowel) {
      let nextToken;
      if (this.tokenUnitIndex < this.numTokenUnits && typeof (nextToken = this.tokenUnits[this.tokenUnitIndex]) !== 'string') {
        if (nextToken.tokenType == TranslitTypes.TokenType.Accent) {
          tokens.accent = nextToken;
          this.tokenUnitIndex++;
        }
      }
    }
    return tokens;
  }

  isEndOfWord() {
    if (this.tokenUnitIndex >= this.numTokenUnits) {
      return true;
    }
    const next = this.tokenUnits[this.tokenUnitIndex];
    if (typeof next === 'string') {
      return TranslitTypes.TamilSuperscripts.indexOf(next) < 0;
    }
    return next.tokenType == TranslitTypes.TokenType.Symbol || next.tokenType == TranslitTypes.TokenType.ToggleTrans;
  }

  previousVisargaConsonant = -1;
  readTamilTokenUnit(start: Token): TokenUnit {
    const tokenUnit: TokenUnit = { leadToken: start };
    if (start.tokenType == TranslitTypes.TokenType.Consonant) {
      const prevVisargaConsonant = this.previousVisargaConsonant;
      const isPrimary = start.idx <= TranslitTypes.SpecialIndices.ப && start.idx % 5 == 0;
      this.previousVisargaConsonant = -1;
      if (isPrimary && !this.options.TamilSuperscripted) {
        if (prevVisargaConsonant != start.idx) {
          if (!this.wordStart) {
            tokenUnit.leadToken = { tokenType: start.tokenType, scriptType: TranslitTypes.ScriptType.Tamil, idx: start.idx + 2 };
          } else if (start.idx == TranslitTypes.SpecialIndices.ச) {
            tokenUnit.leadToken = { tokenType: start.tokenType, scriptType: TranslitTypes.ScriptType.Tamil, idx: TranslitTypes.SpecialIndices.ஸ };
          }
        }
      }

      while (this.tokenUnitIndex < this.numTokenUnits) {
        const nextToken = this.tokenUnits[this.tokenUnitIndex];
        if (typeof nextToken !== 'string') {
          switch (nextToken.tokenType) {
            case TranslitTypes.TokenType.ConsonantDiacritic:
              this.tokenUnitIndex++;
              tokenUnit.consonantDiacritic = nextToken;
              break;
            case TranslitTypes.TokenType.VowelDiacritic:
              this.tokenUnitIndex++;
              if (isPrimary && nextToken.idx == TranslitTypes.SpecialIndices.Virama && !this.options.TamilSuperscripted) {
                this.previousVisargaConsonant = start.idx;
                tokenUnit.leadToken = { tokenType: start.tokenType, scriptType: TranslitTypes.ScriptType.Tamil, idx: start.idx };
                if (this.isEndOfWord()) {
                  return { leadToken: { tokenType: TranslitTypes.TokenType.Ignore, scriptType: TranslitTypes.ScriptType.Tamil, idx: 0 } };
                }
              }
              tokenUnit.vowelDiacritic = nextToken;
              break;
            case TranslitTypes.TokenType.Accent:
              this.tokenUnitIndex++;
              tokenUnit.accent = nextToken;
              break;
            default:
              return tokenUnit;
          }
        } else if ((isPrimary || start.idx == TranslitTypes.SpecialIndices.ஜ) && this.tokenUnitIndex < this.numTokenUnits) {
          const superscriptIndex = TranslitTypes.TamilSuperscripts.indexOf(nextToken);
          if (superscriptIndex > 0) {
            tokenUnit.leadToken = { tokenType: tokenUnit.leadToken.tokenType, scriptType: TranslitTypes.ScriptType.Tamil, idx: start.idx + superscriptIndex }; // no validate
            this.tokenUnitIndex++;
          } else {
            break;
          }
        } else {
          break;
        }
      }
    } else if (start.tokenType == TranslitTypes.TokenType.Vowel && this.tokenUnitIndex < this.numTokenUnits) {
      const nextToken = this.tokenUnits[this.tokenUnitIndex];
      if (typeof nextToken !== 'string') {
        tokenUnit.accent = nextToken;
        this.tokenUnitIndex++;
      }
    }
    return tokenUnit;
  }

  readRomanTokenUnit(start: Token): TokenUnit {
    const tokens: TokenUnit = { leadToken: start };
    if (start.tokenType === TranslitTypes.TokenType.Consonant) {
      let nextToken;
      let vowelAdded = false;
      while (this.tokenUnitIndex < this.numTokenUnits && typeof (nextToken = this.tokenUnits[this.tokenUnitIndex]) !== 'string') {
        let consume = false;
        switch (nextToken.tokenType) {
          case TranslitTypes.TokenType.Vowel:
          case TranslitTypes.TokenType.VowelDiacritic:
            if (!vowelAdded) {

              vowelAdded = true;
              if (nextToken.idx !== TranslitTypes.SpecialIndices.Virama) {
                tokens.vowelDiacritic = { tokenType: TranslitTypes.TokenType.VowelDiacritic, scriptType: TranslitTypes.ScriptType.Roman, idx: nextToken.idx };
              }
              consume = true;
            }
            break;
          case TranslitTypes.TokenType.ConsonantDiacritic:
            tokens.consonantDiacritic = nextToken;
            consume = true;
            break;
          case TranslitTypes.TokenType.Accent:
            tokens.accent = nextToken;
            consume = true;
            break;
        }
        if (!consume) {
          break;
        }
        this.tokenUnitIndex++;
      }
      if (!vowelAdded) {
        tokens.vowelDiacritic = { tokenType: TranslitTypes.TokenType.VowelDiacritic, scriptType: TranslitTypes.ScriptType.Roman, idx: 0 };
      }
    }
    return tokens;
  }
}

class OutputWriter {
  buffer: string[] = [];
  map: ScriptWriterMap;
  options: TranslitOptions;

  constructor(map: ScriptWriterMap, options: TranslitOptions) {
    this.map = map;
    this.options = options;
  }

  writeTokenUnit(tokenUnitOrString: string | TokenUnit) {
    const anuswaraPosition = this.previousAnuswaraPosition;
    this.previousAnuswaraPosition = undefined;
    if (typeof tokenUnitOrString === 'string') {
      if (anuswaraPosition != undefined && this.options.InferAnuswara) {
        this.inferAnuswara(anuswaraPosition, TranslitTypes.SpecialIndices.प);
      }
      this.push(tokenUnitOrString);
      this.wordStart = true;
    } else {
      if (tokenUnitOrString.leadToken.tokenType == TranslitTypes.TokenType.Ignore) {
        return;
      }
      if (tokenUnitOrString.leadToken.tokenType == TranslitTypes.TokenType.Symbol && tokenUnitOrString.leadToken.idx >= TranslitTypes.SpecialIndices.ZeroWidthSpace && !this.options.RetainZeroWidthChars) {
        return;
      }
      switch (this.map.type) {
        case TranslitTypes.ScriptType.Brahmi:
          this.writeBrahmiTokenUnit(tokenUnitOrString);
          break;
        case TranslitTypes.ScriptType.Tamil:
          this.writeTamilTokenUnit(tokenUnitOrString, anuswaraPosition);
          break;
        case TranslitTypes.ScriptType.Roman:
          this.writeRomanTokenUnit(tokenUnitOrString, anuswaraPosition);
          break;
        default:
          return;
      }
      this.wordStart = (tokenUnitOrString.leadToken.tokenType == TranslitTypes.TokenType.Symbol);
    }
  }

  text(): string {
    if (this.previousAnuswaraPosition != undefined && this.options.InferAnuswara) {
      this.inferAnuswara(this.previousAnuswaraPosition, TranslitTypes.SpecialIndices.प);
    }
    return this.buffer.join('');
  }

  push(text: string) {
    if (this.options.IgnoreQuotedMarkers) {
      text = text.replace(/[ʼˮ]/g, '');
    }
    this.buffer.push(text);
  }

  writeBrahmiTokenUnit(tokenUnit: TokenUnit) {
    if (this.options.ASCIINumerals && tokenUnit.leadToken.tokenType === TranslitTypes.TokenType.Symbol && tokenUnit.leadToken.idx < 10) {
      this.push(String.fromCharCode('0'.charCodeAt(0) + tokenUnit.leadToken.idx));
    } else {
      this.push(this.map.lookup(tokenUnit.leadToken));
    }
    if (tokenUnit.vowelDiacritic) {
      this.push(this.map.lookup(tokenUnit.vowelDiacritic));
    }
    if (tokenUnit.consonantDiacritic) {
      this.push(this.map.lookup(tokenUnit.consonantDiacritic));
    }
    if (tokenUnit.accent && !this.options.IgnoreVedicAccents) {
      this.push(this.map.lookup(tokenUnit.accent));
    }
  }

  previousConsonant = 0;
  previousConsonantPosition = 0;
  modernMap: Record<string, string> = {
    'ஸ': 'ச',
    'ஜ': 'ச³',
    'ஜ²': 'ச⁴'
  }
  wordStart = true;
  writeTamilTokenUnit(tokenUnit: TokenUnit, anuswaraPosition?: number) {
    let leadText = this.map.lookup(tokenUnit.leadToken);
    const leadIdx = tokenUnit.leadToken.idx;

    if (tokenUnit.leadToken.tokenType === TranslitTypes.TokenType.Consonant) {
      if (leadIdx === TranslitTypes.SpecialIndices.ந && !this.wordStart) {
        leadText = 'ன';
      } else if (this.previousConsonant === TranslitTypes.SpecialIndices.ன && Math.floor(leadIdx / 5) === 3) {
        this.buffer[this.previousConsonantPosition] = 'ந';
      } else if (this.options.TamilTraditional) {
        const repl = this.modernMap[leadText];
        if (repl !== undefined) {
          leadText = repl;
        }
      }
      if (anuswaraPosition != undefined && this.options.InferAnuswara) {
        this.inferAnuswara(anuswaraPosition, leadIdx);
      }

      let superscript = '';
      if (leadText.length > 1 && TranslitTypes.TamilSuperscripts.indexOf(leadText.slice(-1)) >= 0) {
        superscript = leadText.slice(-1);
        leadText = leadText.slice(0, -1);
      }
      this.push(leadText);
      const consosantPosition = this.buffer.length;

      if (tokenUnit.vowelDiacritic) {
        this.push(this.map.lookup(tokenUnit.vowelDiacritic));
      }

      if (this.options.TamilSuperscripted && superscript) {
        this.push(superscript);
      }

      if (tokenUnit.consonantDiacritic) {
        const consonantDiacriticText = this.map.lookup(tokenUnit.consonantDiacritic);
        if (tokenUnit.consonantDiacritic.idx === TranslitTypes.SpecialIndices.Anuswara) {
          this.previousAnuswaraPosition = this.buffer.length + this.text.length;
        }
        this.push(consonantDiacriticText);
        this.previousConsonant = -1;
      } else {
        this.previousConsonant = leadIdx;
        this.previousConsonantPosition = consosantPosition;
      }
    } else {
      this.previousConsonant = -1;
      if (this.options.ASCIINumerals && tokenUnit.leadToken.tokenType === TranslitTypes.TokenType.Symbol && leadIdx < 10) {
        this.push(String.fromCharCode('0'.charCodeAt(0) + leadIdx));
      } else {
        this.push(leadText);
      }
    }

    if (!this.options.IgnoreVedicAccents && tokenUnit.accent && (tokenUnit.leadToken.tokenType === TranslitTypes.TokenType.Consonant || tokenUnit.leadToken.tokenType === TranslitTypes.TokenType.Vowel)) {
      const lastTokenText = this.buffer.slice(-1)[0];
      const accent = this.map.lookup(tokenUnit.accent);
      if (TranslitTypes.TamilSpecialChars.includes(lastTokenText.slice(-1))) {
        this.buffer.pop();
        this.buffer.push(lastTokenText.slice(0, -1));
        this.push(accent);
        this.buffer.push(lastTokenText.slice(-1));
      } else {
        this.push(accent);
      }
    }
  }

  inferAnuswara(anuswaraPosition: number, idx: number) {
    let repl = this.map.lookupChar(TranslitTypes.TokenType.Consonant, (idx < TranslitTypes.SpecialIndices.म) ? ((Math.floor(idx / 5) * 5) + 4) : TranslitTypes.SpecialIndices.म);
    if (this.map.type != TranslitTypes.ScriptType.Roman) {
      repl += this.map.lookupChar(TranslitTypes.TokenType.VowelDiacritic, TranslitTypes.SpecialIndices.Virama);
    }
    this.buffer[anuswaraPosition] = repl;
  }

  previousAnuswaraPosition?: number;
  writeRomanTokenUnit(tokenUnit: TokenUnit, anuswaraPosition?: number) {
    const leadToken = tokenUnit.leadToken;
    this.push(this.map.lookup(leadToken));
    if (leadToken.tokenType === TranslitTypes.TokenType.Consonant) {
      if (anuswaraPosition != undefined && this.options.InferAnuswara) {
        this.inferAnuswara(anuswaraPosition, leadToken.idx);
      }
      if (!tokenUnit.vowelDiacritic) {
        this.push(this.map.lookupChar(TranslitTypes.TokenType.Vowel, TranslitTypes.SpecialIndices.Virama));
      } else if (tokenUnit.vowelDiacritic.idx !== TranslitTypes.SpecialIndices.Virama) {
        this.push(this.map.lookup(tokenUnit.vowelDiacritic));
      }
    }
    if (tokenUnit.accent && !this.options.IgnoreVedicAccents) {
      this.push(this.map.lookup(tokenUnit.accent));
    }
    if (tokenUnit.consonantDiacritic) {
      if (tokenUnit.consonantDiacritic.idx == TranslitTypes.SpecialIndices.Anuswara) {
        this.previousAnuswaraPosition = this.buffer.length;
      }
      this.push(this.map.lookup(tokenUnit.consonantDiacritic));
    }
  }
}

function populateScriptDataMap() {
  const scriptDataMap: Map<string, ScriptData> = new Map();
  for (const [scriptType, scripts] of Object.entries(TranslitData.Primary)) {
    for (const [name, scriptInfo] of Object.entries(scripts)) {
      const maps = (scriptInfo as unknown) as string[][];
      const type = TranslitTypes.getScriptTypeFromString(scriptType);
      scriptDataMap.set(name, {
        type,
        charMaps: maps
      });
    }
  }
  return scriptDataMap;
}
const scriptDataMap: Map<string, ScriptData> = populateScriptDataMap();

const readerMapCache: Map<string, ScriptReaderMap> = new Map();
function getInputReader(text: string, from: string, options: TranslitOptions) {
  if (from == 'tamilsuperscripted') {
    options.TamilSuperscripted = true;
    from = 'tamil';
  }
  if (!scriptIsReadable(from)) {
    return;
  }
  let readerMap = readerMapCache.get(from);
  if (readerMap == undefined) {
    if (from == 'indic') {
      const scriptData = scriptDataMap.get('devanagari');
      if (scriptData == undefined) {
        return;
      }
      readerMap = new ScriptReaderMap(from, scriptData);
      readerMapCache.set(from, readerMap);

      for (const [name, scriptData] of scriptDataMap) {
        if (name == 'devanagari' || (scriptData.type != TranslitTypes.ScriptType.Brahmi && scriptData.type != TranslitTypes.ScriptType.Tamil)) {
          continue;
        }

        readerMap.addScript(name, scriptData);
      }
    } else {
      const scriptData = scriptDataMap.get(from);
      if (scriptData == undefined) {
        return;
      }

      readerMap = new ScriptReaderMap(from, scriptData);
      readerMapCache.set(from, readerMap);
    }
  }
  return new InputReader(text, readerMap, options);
}

const writerMapCache: Map<string, ScriptWriterMap> = new Map();
function getOutputWriter(to: string, options: TranslitOptions) {
  switch (to) {
    case 'tamilsimple':
      options.IgnoreQuotedMarkers = true;
      options.InferAnuswara = true;
    // case fall-through
    case 'tamilsuperscripted':
      options.TamilSuperscripted = true;
      to = 'tamil';
      break;
    case 'ipa':
      options.IgnoreVedicAccents = true;
      break;
    case 'easyroman':
      options.InferAnuswara = true;
      break
  }
  let writerMap = writerMapCache.get(to);
  if (writerMap == undefined) {
    const scriptData = scriptDataMap.get(to);
    if (scriptData == undefined) {
      return;
    }
    writerMap = new ScriptWriterMap(to, scriptData);
    writerMapCache.set(to, writerMap);
  }
  return new OutputWriter(writerMap, options);
}

export function transliterate(text: string, from: string, to: string, opts: string): string {
  const options = getTranslitOptions(opts);
  const reader = getInputReader(text, from, options);
  const writer = getOutputWriter(to, options);
  if (reader == undefined || writer == undefined) {
    return text;
  }

  while (reader.hasMore()) {
    writer.writeTokenUnit(reader.getNext());
  }

  return writer.text();
}
