/// <reference types="emscripten" />
/** Above will import declarations from @types/emscripten, including Module etc. */

export enum TranslitOptions {
  None = 0,
  IgnoreVedicAccents = 1,
  IgnoreQuotedMarkers = 2,
  TamilTraditional = 4,
  TamilSuperscripted = 8,
  InferAnuswara = 16,
  RetainZeroWidthChars = 32,
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
