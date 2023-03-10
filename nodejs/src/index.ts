import fs from 'fs';

// Exported functions:
//   memory: Memory [WebAssembly.Memory] {},
//   translitOptionsToInt: [Function: 20],
//   transliterate: [Function: 21],
//   transliterate2: [Function: 22],
//   releaseBuffer: [Function: 23],
//   free: [Function: 42],
//   __indirect_function_table: Table [WebAssembly.Table] {},
//   __errno_location: [Function: 33],
//   malloc: [Function: 41],
//   stackSave: [Function: 60],
//   stackRestore: [Function: 61],
//   stackAlloc: [Function: 62]
interface IndiTransExports {
  transliterate(
    text: number,
    from: number,
    to: number,
    options: number
  ): number;
  releaseBuffer(ptr: number): void;
  stackAlloc(size: number): number;
  stackRestore(ptr: number): void;
  memory: WebAssembly.Memory;
  __indirect_function_table: WebAssembly.Table;
}

export enum TranslitOptions {
  None = 0,
  IgnoreVedicAccents = 1,
  IgnoreQuotedMarkers = 2,
  TamilTraditional = 4,
  TamilSuperscripted = 8,
  RetainZeroWidthChars = 16,
  ASCIINumerals = 32,
}

export class IndiTrans {
  static instance = IndiTrans._init();

  private static async _init(): Promise<IndiTrans> {
    const { instance } = await WebAssembly.instantiate(
      fs.readFileSync(`${__dirname}/inditrans.wasm`)
    );
    return new IndiTrans(instance.exports as unknown as IndiTransExports);
  }

  private module: IndiTransExports;
  private memory: Uint8Array;

  private constructor(wrapper: IndiTransExports) {
    this.module = wrapper;
    this.memory = new Uint8Array(wrapper.memory.buffer);
  }

  private stringToNativeArray(
    str: string,
    outIdx: number,
    maxBytesToWrite: number
  ): number {
    if (!(maxBytesToWrite > 0)) return 0;
    const startIdx = outIdx;
    const endIdx = outIdx + maxBytesToWrite - 1;
    for (let i = 0; i < str.length; ++i) {
      let charCode = str.charCodeAt(i);
      if (charCode >= 55296 && charCode <= 57343) {
        const byte1 = str.charCodeAt(++i);
        charCode = (65536 + ((charCode & 1023) << 10)) | (byte1 & 1023);
      }
      if (charCode <= 127) {
        if (outIdx >= endIdx) break;
        this.memory[outIdx++] = charCode;
      } else if (charCode <= 2047) {
        if (outIdx + 1 >= endIdx) break;
        this.memory[outIdx++] = 192 | (charCode >> 6);
        this.memory[outIdx++] = 128 | (charCode & 63);
      } else if (charCode <= 65535) {
        if (outIdx + 2 >= endIdx) break;
        this.memory[outIdx++] = 224 | (charCode >> 12);
        this.memory[outIdx++] = 128 | ((charCode >> 6) & 63);
        this.memory[outIdx++] = 128 | (charCode & 63);
      } else {
        if (outIdx + 3 >= endIdx) break;
        this.memory[outIdx++] = 240 | (charCode >> 18);
        this.memory[outIdx++] = 128 | ((charCode >> 12) & 63);
        this.memory[outIdx++] = 128 | ((charCode >> 6) & 63);
        this.memory[outIdx++] = 128 | (charCode & 63);
      }
    }
    this.memory[outIdx] = 0;
    return outIdx - startIdx;
  }

  private nativeArrayToString(idx: number, maxBytesToRead: number) {
    const endIdx = idx + maxBytesToRead;
    let endPtr = idx;
    while (this.memory[endPtr] && !(endPtr >= endIdx)) ++endPtr;
    let str = '';
    while (idx < endPtr) {
      let byte0 = this.memory[idx++];
      if (!(byte0 & 128)) {
        str += String.fromCharCode(byte0);
        continue;
      }
      const byte1 = this.memory[idx++] & 63;
      if ((byte0 & 224) == 192) {
        str += String.fromCharCode(((byte0 & 31) << 6) | byte1);
        continue;
      }
      const byte2 = this.memory[idx++] & 63;
      if ((byte0 & 240) == 224) {
        byte0 = ((byte0 & 15) << 12) | (byte1 << 6) | byte2;
      } else {
        byte0 =
          ((byte0 & 7) << 18) |
          (byte1 << 12) |
          (byte2 << 6) |
          (this.memory[idx++] & 63);
      }
      if (byte0 < 65536) {
        str += String.fromCharCode(byte0);
      } else {
        const ch = byte0 - 65536;
        str += String.fromCharCode(55296 | (ch >> 10), 56320 | (ch & 1023));
      }
    }
    return str;
  }

  private stringToUtf8(str: string): number {
    let ret = 0;
    if (str !== null && str !== undefined) {
      const len = (str.length << 2) + 1;
      ret = this.module.stackAlloc(len);
      this.stringToNativeArray(str, ret, len);
    }
    return ret;
  }

  transliterate(
    text: string,
    from: string,
    to: string,
    options: TranslitOptions
  ): string {
    const pointers = [text, from, to].map((str) => this.stringToUtf8(str));
    const resultPtr = this.module.transliterate(
      pointers[0],
      pointers[1],
      pointers[2],
      options
    );
    let result = '';
    if (resultPtr) {
      let ptr = resultPtr;
      while (this.memory[ptr]) {
        ptr++;
      }
      result = this.nativeArrayToString(resultPtr, ptr - resultPtr);
      this.module.releaseBuffer(resultPtr);
    }
    pointers.map((ptr) => this.module.stackRestore(ptr));

    return result;
  }
}
