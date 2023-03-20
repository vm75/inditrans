#!/bin/bash

init_emcc() {
  # check if EMSDK is not defined or if the path does not exist
  if [[ -z "${EMSDK}" || ! -d "${EMSDK}" ]]; then
    # look for emsdk in the default location - AppData\Local\Programs\emsdk
    emsdk=~/.local/share/emsdk

    # if not installed clone https://github.com/emscripten-core/emsdk.git and install
    if [[ ! -d "${emsdk}" ]]; then
      mkdir -p ~/.local/share
      git clone https://github.com/emscripten-core/emsdk.git ${emsdk}

      currDir=$(pwd)

      cd ${emsdk} || exit
      ./emsdk install latest
      ./emsdk activate latest
      export EMSDK=${emsdk}
    else
      # set EMSDK environment variable and persist it
      export EMSDK=${emsdk}
    fi
  fi

  # set emsdk environment variables
  export EMSDK_QUIET=1
  source ${EMSDK}/emsdk_env.sh
}

# create a function
build_wasm_standalone() {
  exportedFunctions='["_malloc", "_free", "_transliterate", "_isScriptSupported", "_releaseBuffer"]'

  # get the path to the output directory
  outDir='./flutter/assets'

  # create the output directory if it does not exist
  if [[ ! -d "${outDir}" ]]; then
    mkdir -p "${outDir}"
  fi

  # build the function
  if [[ $2 == "debug" ]]; then
    emcc ./native/src/inditrans.cpp -I ./native/src \
      -std=c++20 -g3 --profiling-funcs -s ASSERTIONS=1 -fsanitize=address \
      "-Wl,--no-entry" \
      -DDEBUG \
      -s EXPORTED_FUNCTIONS="${exportedFunctions}" \
      -s FILESYSTEM=0 \
      -o "${outDir}/inditrans.wasm"
  else
    emcc ./native/src/inditrans.cpp -I ./native/src \
      -std=c++20 -Oz -fno-exceptions -fno-rtti -fno-stack-protector -ffunction-sections -fdata-sections -fno-math-errno \
      "-Wl,--gc-sections,--no-entry" \
      -DNDEBUG \
      -s EXPORTED_FUNCTIONS="${exportedFunctions}" \
      -s FILESYSTEM=0 \
      -o "${outDir}/inditrans.wasm"
  fi
}

build_wasm_js() {
  exportedRuntimeMethods='["ccall", "cwrap"]'
  exportedFunctions='["_malloc", "_free", "_transliterate", "_isScriptSupported", "_releaseBuffer"]'

  # get the path to the output directory
  outDir='./js/public'

  # create the output directory if it does not exist
  if [ ! -d "$outDir" ]; then
    mkdir -p "$outDir"
  fi

  # build the function
  if [ "$1" == "debug" ]; then
    emcc ./native/src/inditrans.cpp -I ./native/src \
      -std=c++20 -g3 --profiling-funcs -s ASSERTIONS=1 -fsanitize=address \
      "-Wl,--no-entry" \
      -DDEBUG \
      -s EXPORTED_FUNCTIONS="$exportedFunctions" \
      -s EXPORTED_RUNTIME_METHODS="$exportedRuntimeMethods" \
      -s WASM=1 \
      -s ENVIRONMENT='web,node' \
      -s SINGLE_FILE=1 \
      -s ALLOW_MEMORY_GROWTH=1 \
      -s NO_EXIT_RUNTIME=1 \
      -s FILESYSTEM=0 \
      --post-js ./js/src/inditrans.post.js \
      -o "$outDir/inditrans.js"
  else
    emcc ./native/src/inditrans.cpp -I ./native/src \
      -std=c++20 -Oz -fno-exceptions -fno-rtti -fno-stack-protector -ffunction-sections -fdata-sections -fno-math-errno \
      "-Wl,--gc-sections,--no-entry" \
      -DNDEBUG \
      -s EXPORTED_FUNCTIONS="$exportedFunctions" \
      -s EXPORTED_RUNTIME_METHODS="$exportedRuntimeMethods" \
      -s WASM=1 \
      -s ENVIRONMENT='web,node' \
      -s SINGLE_FILE=1 \
      -s ALLOW_MEMORY_GROWTH=1 \
      -s NO_EXIT_RUNTIME=1 \
      -s FILESYSTEM=0 \
      --post-js ./js/src/inditrans.post.js \
      -o "$outDir/inditrans.js"
  fi
}

# initialize emcc
init_emcc

cd $(dirname ${BASH_SOURCE[0]})/..

# build
if [ "$1" == "standalone" ]; then
  build_wasm_standalone "$2"
elif [ "$1" == "js" ]; then
  build_wasm_js "$2"
else
  echo "Invalid build target"
fi
