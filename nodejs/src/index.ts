import { Option } from './Option';
import { Script } from './Script';
import path from 'path';

let Module: any = null;

export { Option, Script };

export async function init() {
  // Try to load from compiled dist location first, fall back to src
  let inditransPath: string;
  try {
    inditransPath = path.join(__dirname, 'inditrans.js');
  } catch {
    inditransPath = path.join(__dirname, '../js/public/inditrans.js');
  }

  // The module directly exports Module, not a factory
  Module = require(inditransPath);

  // Wait for runtime initialization if needed
  if (Module.onRuntimeInitialized) {
    await new Promise(resolve => {
      if (Module.inditrans) {
        resolve(undefined);
      } else {
        const originalCallback = Module.onRuntimeInitialized;
        Module.onRuntimeInitialized = () => {
          if (originalCallback) originalCallback();
          resolve(undefined);
        };
      }
    });
  }
}

export function transliterate(
  text: string,
  from: Script,
  to: Script,
  options: Option
): string {
  if (!Module) {
    throw new Error('Inditrans not initialized. Call init() first.');
  }
  return Module.inditrans.transliterate(text, from, to, options);
}

export function isScriptSupported(script: string): boolean {
  if (!Module) {
    throw new Error('Inditrans not initialized. Call init() first.');
  }
  return Module.inditrans.isScriptSupported(script);
}

export default { Option, Script, init, transliterate, isScriptSupported };
