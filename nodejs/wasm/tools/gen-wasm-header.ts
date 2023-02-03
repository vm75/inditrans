import TranslitData from '../../assets/scripts.json' assert { type: 'json' };
import fs from 'fs';

type ScriptInfo = {
  name: string,
  type: string,
  info: string[][]
};

function writeFileHeader(script: fs.WriteStream) {
  script.write(`#pragma once

#include <array>
#include <string>

enum class ScriptType : uint8_t { Tamil, Brahmi, Roman, Others };

struct ScriptData {
  ScriptType type {};
  std::array<std::string_view, 19> vowels;
  std::array<std::string_view, 19> vowelDiacritics;
  std::array<std::string_view, 50> consonants;
  std::array<std::string_view, 5> consonantDiacritic;
  std::array<std::string_view, 19> symbols;
};

// clang-format off
constexpr std::array<std::pair<std::string_view, ScriptData>, 118> ScriptDataMap {{
`)
}

function writeFileFooter(script: fs.WriteStream) {
  script.write(`}};
// clang-format on
`);
}

function writeArray(script: fs.WriteStream, arr: string[]) {
  script.write('    { ');
  let first = true;
  arr.forEach(entry => {
    if (!first) {
      script.write(', ');
    }
    first = false;
    script.write('"');
    entry = entry.replace(/"/g, () => {
      return '\\"';
    })
    script.write(entry);
    script.write('"');
  });
  script.write(' }');
}

function writeScript(script: fs.WriteStream, scriptInfo: ScriptInfo, first: boolean) {
  if (!first) {
    script.write(',\n');
  }
  first = false;
  script.write('  { "');
  script.write(scriptInfo.name);
  script.write('", {\n');
  script.write('    ScriptType::');
  script.write(scriptInfo.type);
  script.write(',\n');
  let firstArray = true;
  scriptInfo.info.forEach(arr => {
    if (!firstArray) {
      script.write(',\n');
    }
    firstArray = false;
    writeArray(script, arr);
  });
  script.write('\n  }}');
}

function scriptComparator(a: ScriptInfo, b: ScriptInfo) {
  if (a.name < b.name) {
    return -1;
  }
  if (a.name > b.name) {
    return 1;
  }
  return 0;
}

function generateScripts(path: string) {
  const script = fs.createWriteStream(path, { encoding: 'utf8' });

  const primary: ScriptInfo[] = [];
  for (const [type, scripts] of Object.entries(TranslitData.Primary)) {
    for (const [name, info] of Object.entries(scripts)) {
      primary.push({
        name,
        type,
        info
      })
    }
  }

  const secondary: ScriptInfo[] = [];
  for (const [type, scripts] of Object.entries(TranslitData.Secondary)) {
    for (const [name, info] of Object.entries(scripts)) {
      secondary.push({
        name,
        type,
        info
      })
    }
  }

  writeFileHeader(script);

  primary.sort(scriptComparator);
  secondary.sort(scriptComparator);
  let first = true;
  primary.forEach(scriptInfo => {
    writeScript(script, scriptInfo, first);
    first = false;
  })
  script.write(`,
#ifndef __EMSCRIPTEN__
`);
  first = true;
  secondary.forEach(scriptInfo => {
    writeScript(script, scriptInfo, first);
    first = false;
  })

  script.write(`
#endif
`);

  writeFileFooter(script)

  script.close();
}

if (process.argv.length > 2) {
  generateScripts(process.argv[2])
}
