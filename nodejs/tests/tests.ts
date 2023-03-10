import { IndiTrans } from '../src/IndiTrans';
import Tests from './test-cases.json';
import { TranslitOptions } from '../src/TranslitOptions';

function getOptionsMap(): Map<string, TranslitOptions> {
  const map = new Map<string, TranslitOptions>();
  const optionStrings = Object.values(TranslitOptions).filter((value) =>
    isNaN(Number(value))
  ) as string[];
  const optionValue = Object.values(TranslitOptions).filter(
    (value) => !isNaN(Number(value))
  ) as TranslitOptions[];
  for (let i = 0; i < optionStrings.length; i++) {
    map.set(optionStrings[i], optionValue[i]);
  }
  return map;
}

function getOptions(options: string): TranslitOptions {
  const optionsMap = getOptionsMap();
  let result = TranslitOptions.None;
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

let inditrans: IndiTrans;
beforeAll(async () => {
  inditrans = await IndiTrans.instance;
});

test.each(testData)(
  '$description',
  ({ text, fromScript, toScript, options, expected }) => {
    const result = inditrans.transliterate(
      text,
      fromScript,
      toScript,
      getOptions(options)
    );
    expect(result).toBe(expected);
  }
);
