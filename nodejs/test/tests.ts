import * as inditrans from '../src';
import Tests from './test-cases.json';
import { readFileSync } from 'fs';
import path from 'path';

function getOptionsMap(): Map<string, inditrans.Option> {
  const map = new Map<string, inditrans.Option>();
  const optionStrings = Object.values(inditrans.Option).filter((value) =>
    isNaN(Number(value))
  ) as string[];
  const optionValue = Object.values(inditrans.Option).filter(
    (value) => !isNaN(Number(value))
  ) as inditrans.Option[];
  for (let i = 0; i < optionStrings.length; i++) {
    map.set(optionStrings[i], optionValue[i]);
  }
  return map;
}

function getOptions(options: string): inditrans.Option {
  const optionsMap = getOptionsMap();
  let result = inditrans.Option.None;
  options.split(' ').forEach((option) => {
    result += optionsMap.get(option) || 0;
  });

  return result;
}

class TestData {
  description: string;
  text: string;
  fromScript: string;
  toScript: string;
  options: string;
  expected: string;

  constructor(
    description: string,
    text: string,
    fromScript: string,
    toScript: string,
    options: string,
    expected: string
  ) {
    this.description = description;
    this.text = text;
    this.fromScript = fromScript;
    this.toScript = toScript;
    this.options = options;
    this.expected = expected;
  }
}

const testData = [];

for (const input of Tests) {
  for (const target of input.targets) {
    testData.push(
      new TestData(
        input.description,
        input.text,
        input.script,
        target.script,
        target.options || '',
        target.text
      )
    );
  }
}

beforeAll(async () => {
  await inditrans.init();
});

test('optional Urdu pronunciation pack and fallback', () => {
  const lexicon = readFileSync(path.join(__dirname, '../../test-files/urdu-test-lexicon.iul'));
  expect(inditrans.transliterateUrduWithLexicon('کتاب', inditrans.Script.iast, lexicon)).toBe('kitāb');
  expect(inditrans.transliterateUrduWithLexicon('##کتاب## کتاب', inditrans.Script.iast, lexicon))
    .toBe('کتاب kitāb');
  const reused = new inditrans.UrduPronunciationLexicon(lexicon);
  try {
    expect(reused.transliterate('کتاب', inditrans.Script.iast)).toBe('kitāb');
    expect(reused.transliterate('کتاب۔کتاب', inditrans.Script.iast))
      .toBe('kitāb' + inditrans.transliterate('۔', inditrans.Script.urdu, inditrans.Script.iast, inditrans.Option.None) + 'kitāb');
    expect(reused.transliterate('کتاب', inditrans.Script.iast, inditrans.Option.DisableInference))
      .toBe(inditrans.transliterate('کتاب', inditrans.Script.urdu, inditrans.Script.iast, inditrans.Option.DisableInference));
  } finally {
    reused.dispose();
  }
  expect(() => reused.transliterate('کتاب', inditrans.Script.iast)).toThrow();
  expect(() => new inditrans.UrduPronunciationLexicon(lexicon.subarray(1))).toThrow();
  const fallback = inditrans.transliterate('کتاب', inditrans.Script.urdu, inditrans.Script.iast, inditrans.Option.None);
  expect(inditrans.transliterateUrduWithLexicon('کتاب', inditrans.Script.iast, lexicon.subarray(1))).toBe(fallback);
  expect(inditrans.transliterateUrduWithLexicon('کتاب', inditrans.Script.iast, lexicon, inditrans.Option.DisableInference))
    .toBe(inditrans.transliterate('کتاب', inditrans.Script.urdu, inditrans.Script.iast, inditrans.Option.DisableInference));
});

test.each(testData)(
  '$description',
  ({ text, fromScript, toScript, options, expected }) => {
    const result = inditrans.transliterate(
      text,
      <inditrans.Script>fromScript,
      <inditrans.Script>toScript,
      getOptions(options)
    );
    expect(result).toBe(expected);
  }
);
