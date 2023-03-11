import { Inditrans } from './Inditrans';
import { Option } from './Option';
import { Script } from './Script';

let inditrans: Inditrans | null = null;

export { Option, Script };

export async function init() {
  inditrans = await Inditrans.instance;
}

export function transliterate(
  text: string,
  from: Script,
  to: Script,
  options: Option
): string {
  if (!inditrans) {
    throw new Error('Inditrans not initialized');
  }
  return inditrans.transliterate(text, from, to, options);
}

export default [Option, Script, init, transliterate];
