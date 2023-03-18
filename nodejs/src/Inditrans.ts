import { InditransModule } from './InditransModule';
import { Option } from './Option';
import { Script } from './Script';
import fs from 'fs';

export class Inditrans {
  static instance = Inditrans._init();

  private static async _init(): Promise<Inditrans> {
    const { instance } = await WebAssembly.instantiate(
      fs.readFileSync(`${__dirname}/inditrans.wasm`)
    );
    return new Inditrans(instance.exports as unknown as InditransModule);
  }

  private module: InditransModule;
  private memory: Uint8Array;

  private constructor(wrapper: InditransModule) {
    this.module = wrapper;
    this.memory = new Uint8Array(wrapper.memory.buffer);
  }

  private utf82ToString(ptr: number): string {
    if (ptr === 0) {
      return '';
    }
    const decoder = new TextDecoder();
    let endPtr = ptr;
    while (this.memory[endPtr]) {
      ++endPtr;
    }
    return decoder.decode(this.memory.subarray(ptr, endPtr));
  }

  private stringToUtf8(str: string): number {
    let ret = 0;
    if (str === null && str === undefined) {
      return ret;
    }
    const encoder = new TextEncoder();
    const encodedText = encoder.encode(str);
    const len = encodedText.length + 1;
    ret = this.module.stackAlloc(len);
    if (encodedText.length > 0) {
      this.memory.set(encodedText, ret);
    }
    this.memory[ret + encodedText.length] = 0;
    return ret;
  }

  transliterate(
    text: string,
    from: Script,
    to: Script,
    options: Option
  ): string {
    const pointers = [text, from, to].map((str) =>
      this.stringToUtf8(str.toString())
    );
    const resultPtr = this.module.transliterate(
      pointers[0],
      pointers[1],
      pointers[2],
      options
    );
    let result = '';
    if (resultPtr !== 0) {
      result = this.utf82ToString(resultPtr);
      this.module.releaseBuffer(resultPtr);
    }
    pointers.map((ptr) => this.module.stackRestore(ptr));

    return result;
  }

  isScriptSupported(script: string): boolean {
    const ptr = this.stringToUtf8(script);
    const result = this.module.isScriptSupported(ptr);
    this.module.stackRestore(ptr);
    return result == 1;
  }
}
