// Exported functions:
//   memory: WebAssembly.Memory
//   transliterate
//   releaseBuffer
//   free
//   __indirect_function_table: WebAssembly.Table
//   __errno_location
//   malloc
//   stackSave
//   stackRestore
//   stackAlloc
export interface InditransModule {
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
