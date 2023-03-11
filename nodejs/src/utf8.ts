import { InditransModule } from './InditransModule';

export class Codec {
  private module: InditransModule;
  private memory: Uint8Array;

  constructor(module: InditransModule) {
    this.module = module;
    this.memory = new Uint8Array(module.memory.buffer);
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

  private stringToUtf82(str: string): number {
    let ret = 0;
    if (str !== null && str !== undefined) {
      const len = (str.length << 2) + 1;
      ret = this.module.stackAlloc(len);
      this.stringToNativeArray(str, ret, len);
    }
    return ret;
  }
}
