/// <reference types="emscripten" />
/** Above will import declarations from @types/emscripten, including Module etc. */

export class TranslitOptions {
  public static readonly None = 0;
  public static readonly IgnoreVedicAccents = 1;
  public static readonly IgnoreQuotedMarkers = 2;
  public static readonly TamilTraditional = 4;
  public static readonly TamilSuperscripted = 8;
  public static readonly InferAnuswara = 16;
  public static readonly RetainZeroWidthChars = 32;
}

// This will merge to the existing EmscriptenModule interface from @types/emscripten
// If this doesn't work, try globalThis.EmscriptenModule instead.
export interface InditransModule extends EmscriptenModule {
  // Module.cwrap() will be available by doing this.
  // Requires -s "EXTRA_EXPORTED_RUNTIME_METHODS=['cwrap']"
  cwrap: typeof cwrap;
  // Exported from inditrans.cpp
  // Requires "EXPORTED_FUNCTIONS=['_translit', ...]"
  _translitOptionsToInt(options: string): number;
  _transliterate(text: string, from: string, to: string, options: number): number;
  _transliterate2(text: string, from: string, to: string, options: string): number;
  _releaseBuffer(buffer: number): string;
  // or using cwrap. See below
  translitOptionsToInt(options: string): number;
  transliterate(text: string, from: string, to: string, options: number): string;
  transliterate2(text: string, from: string, to: string, options: string): string;
}

export default function InditransWasm(mod?: any): Promise<InditransModule>;