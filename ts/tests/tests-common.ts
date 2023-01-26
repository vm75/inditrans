import Tests from '../tests.json' assert { type: 'json' };
import fs from 'fs';

const perfTestFile = 'dist/perf.txt';

export interface TestHelper {
  transliterate: (text: string, from: string, to: string, optStr: string) => string;
  setErrorLogger: (errorLogger: (text: string) => void) => void;
}

function consoleLogger(text: string) {
  console.log(text);
}

export function testTranslit(translitWrapper: TestHelper, testNum: number, desc: string, from: string, to: string, text: string, expected: string, optStr: string) {
  const res = translitWrapper.transliterate(text, from, to, optStr);
  const result = res === expected;
  console.log(`  Test #${testNum.toString()}: ${desc}: (${from} -> ${to}, options=${optStr}): ${result ? ' PASSED' : 'FAILED'}`);
  if (!result) {
    console.log(`    Input   : ${text}`);
    console.log(`    Expected: ${expected}`);
    console.log(`    Observed: ${res}`);
  }

  return result;
}

export function timeTranslit(translitWrapper: TestHelper, text: string, from: string, to: string, optStr: string): string {
  const start = Date.now();
  translitWrapper.transliterate(text, from, to, optStr);
  const millis = Date.now() - start;
  return millis.toString();
}


export function testAllTranslit(translitWrapper: TestHelper) {
  let testNum = 1;
  let failedCount = 0;
  console.log('Inditrans tests');
  Tests.forEach(input => {
    input.targets.forEach(target => {
      failedCount += testTranslit(translitWrapper, testNum++, input.description, input.script, target.script, input.text, target.text, target.options || '') ? 0 : 1;
    })
  })

  console.log(`Summary: ${failedCount.toString()} tests failed.`);
}

export function testPerf(translitWrapper: TestHelper) {
  try {
    const inBuffer = fs.readFileSync(perfTestFile, 'utf8');
    console.log('Perf test');
    console.log(`  Transliterate to telugu: ${timeTranslit(translitWrapper, inBuffer, 'devanagari', 'telugu', '')} ms`);
    console.log(`  Transliterate to tamil: ${timeTranslit(translitWrapper, inBuffer, 'devanagari', 'tamil', '')} ms`);
    console.log(`  Transliterate to roman: ${timeTranslit(translitWrapper, inBuffer, 'devanagari', 'iso', '')} ms`);
  } catch (error) {
    console.log(error);
  }
}

export function testAll(translitWrapper: TestHelper) {
  translitWrapper.setErrorLogger(consoleLogger);
  testAllTranslit(translitWrapper);
  testPerf(translitWrapper);
}
