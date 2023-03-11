import * as inditrans from '../src';
import Tests from './test-cases.json';

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
