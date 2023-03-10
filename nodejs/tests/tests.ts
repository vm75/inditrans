import { IndiTrans, TranslitOptions } from '../src';
import Tests from './test-cases.json';

async function withIndiTrans(callback: (indiTrans: IndiTrans) => void) {
  IndiTrans.instance.then((indiTrans) => {
    callback(indiTrans);
  });
}

function getOptionsMap(): Map<string, TranslitOptions> {
  const map = new Map<string, TranslitOptions>();
  const optionStrings = Object.values(TranslitOptions).filter((value) =>
    isNaN(Number(value))
  ) as string[];
  const optionValue = Object.values(TranslitOptions).filter(
    (value) => !isNaN(Number(value))
  ) as TranslitOptions[];
  for (let i = 0; i < optionStrings.length; i++) {
    map.set(optionStrings[i], optionValue[i]);
  }
  return map;
}

function getOptions(options: string): TranslitOptions {
  const optionsMap = getOptionsMap();
  let result = TranslitOptions.None;
  options.split(' ').forEach((option) => {
    result += optionsMap.get(option) || 0;
  });

  return result;
}

try {
  IndiTrans.instance.then((inditrans) => {
    for (const input of Tests) {
      for (const target of input.targets) {
        test(input.description, (done) => {
          const result = inditrans.transliterate(
            input.text,
            input.script,
            target.script,
            getOptions(target.options || '')
          );
          // console.log(result);
          expect(result).toBe(target.text);
          done();
        });
      }
    }
  });
} catch (error) {
  console.error('*********************');
}

// test('Calling wasm methods', async () => {
//   await withIndiTrans((inditrans) => {
//     expect(
//       inditrans.transliterate(
//         'தங்கப் பதக்கம்',
//         'tamil',
//         'readablelatin',
//         TranslitOptions.None
//       )
//     ).toBe('tanga padakkam');
//   });
// });
