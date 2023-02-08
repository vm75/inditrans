import { TestHelper, testAll } from './test-utils';
import InditransWasm, { InditransModule } from '../wasm/src/inditrans';

class TranslitProxy implements TestHelper {
  module: InditransModule;
  constructor(module: InditransModule) {
    this.module = module;
  }
  transliterate(text: string, from: string, to: string, optStr: string) {
    return this.module.transliterate(text, from, to, this.module.translitOptionsToInt(optStr));
  }
  setErrorLogger(_errorLogger: (text: string) => void) {
    //
  }
}

const translitCpp = await InditransWasm();

testAll(new TranslitProxy(translitCpp));