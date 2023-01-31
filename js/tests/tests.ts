import { TestHelper, testAll } from './tests-common';
import { setErrorLogger } from '../../ts/src/inditrans';
import { transliterate } from '../../ts/src/index';

class TranslitProxy implements TestHelper {
  transliterate(text: string, from: string, to: string, optStr: string) {
    return transliterate(text, from, to, optStr);
  }
  setErrorLogger(errorLogger: (text: string) => void) {
    setErrorLogger(errorLogger);
  }
}

testAll(new TranslitProxy());
