#!/bin/bash

script=$(realpath "${0}")
scriptDir=$(dirname "${script}")
distDir=$(realpath "${scriptDir}/../../dist")
source="${scriptDir}/src"/*.cpp

mkdir "${distDir}" 2> /dev/null

buildWasm() {
  wasmDir="${distDir}/wasm/src"
  if [[ $1 == -d ]] ; then
    cppoptions="-std=c++20 -g3 --profiling-funcs -s ASSERTIONS=1 -fsanitize=address"
  else
    cppoptions="-std=c++20 -O3 -fno-exceptions -fno-rtti -fno-stack-protector -ffunction-sections -fdata-sections -fno-math-errno -Wl,--gc-sections"
  fi
  if [[ ! -d ${wasmDir} ]] ; then
    mkdir -p ${wasmDir}
  fi
  docker run --rm -v "${scriptDir}/src:/src" -v "${wasmDir}:/src/out" -u $(id -u):$(id -g) \
    emscripten/emsdk emcc inditrans.cpp -o "out/indic_trans_wasm.js" ${cppoptions} \
    -s WASM=1 \
    -s ENVIRONMENT='web,node' \
    -s NO_EXIT_RUNTIME=1 \
    -s MODULARIZE=1 \
    -s EXPORT_ES6=1 \
    -s EXPORT_NAME=InditransWasm \
    -s FILESYSTEM=0 \
    -s SINGLE_FILE=1 \
    -s ALLOW_MEMORY_GROWTH=1 \
    -s 'EXPORTED_RUNTIME_METHODS=["cwrap", "lengthBytesUTF8", "stringToUTF8"]' \
    -s 'EXPORTED_FUNCTIONS=["_translitOptionsToInt", "_transliterate", "_transliterate2", "_releaseBuffer", "_malloc", "_free"]' \
    --extern-pre-js indic_trans_wasm.extern-pre.js \
    --pre-js indic_trans_wasm.pre.js \
    --post-js indic_trans_wasm.post.js
}

buildNative() {
  targetBin="${distDir}/indic_trans_test"
  if [[ $1 == -d ]] ; then
    cppoptions="-std=c++20 -O3 -fno-exceptions"
  else
    cppoptions="-std=c++20 -O3 -fno-exceptions -fno-rtti -fno-stack-protector -ffunction-sections -fdata-sections -fno-math-errno -Wl,--gc-sections"
  fi
  rm -f "${targetBin}"
  clang++ ${cppoptions} -I "${scriptDir}/src" "${scriptDir}/src"/*.cpp "${scriptDir}/test"/*.cpp -o "${targetBin}"
  if [[ -f "${targetBin}" ]] ; then
    strip -s -R .comment -R .gnu.version --strip-unneeded "${targetBin}"
  fi
}

if [[ $1 == -a || $1 == -w ]] ; then
  buildWasm
fi
if [[ $1 == -a || $1 == -n ]] ; then
  buildNative
fi
