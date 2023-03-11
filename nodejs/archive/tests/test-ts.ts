import { TestHelper, testAll } from './test-utils';
import { setErrorLogger } from '../ts/src/inditrans';
import { transliterate } from '../ts/src/index';

export interface TestHelper {
  transliterate: (text: string, from: string, to: string, optStr: string) => string;
  setErrorLogger: (errorLogger: (text: string) => void) => void;
}


class TranslitProxy implements TestHelper {
  transliterate(text: string, from: string, to: string, optStr: string) {
    return transliterate(text, from, to, optStr);
  }
  setErrorLogger(errorLogger: (text: string) => void) {
    setErrorLogger(errorLogger);
  }
}

testAll(new TranslitProxy());
