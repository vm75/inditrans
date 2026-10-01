#!/bin/bash
REQUIRED_EMSDK_VERSION="6.0.10"

init_emcc() {
  # check if EMSDK is not defined or if the path does not exist
  if [[ -z "${EMSDK}" || ! -d "${EMSDK}" ]]; then
    # look for emsdk in the default location - ~/.local/share/emsdk
    emsdk=~/.local/share/emsdk

    # if not installed clone https://github.com/emscripten-core/emsdk.git and install
    if [[ ! -d "${emsdk}" ]]; then
      mkdir -p ~/.local/share
      git clone https://github.com/emscripten-core/emsdk.git "${emsdk}"

      currDir=$(pwd)

      cd "${emsdk}" || exit 1
      ./emsdk install "${REQUIRED_EMSDK_VERSION}"
      ./emsdk activate "${REQUIRED_EMSDK_VERSION}"
      export EMSDK="${emsdk}"
      cd "${currDir}"
    else
      # set EMSDK environment variable and persist it
      export EMSDK="${emsdk}"
    fi
  fi

  # set emsdk environment variables
  if [[ -f "${EMSDK}/emsdk_env.sh" ]]; then
    export EMSDK_QUIET=1
    source "${EMSDK}/emsdk_env.sh"
  fi

  # Verify active compiler
  if ! command -v em++ >/dev/null 2>&1; then
    echo "inditrans requires Emscripten ${REQUIRED_EMSDK_VERSION} (em++ not found in PATH or EMSDK)" >&2
    exit 1
  fi

  active_version=$(em++ --version 2>/dev/null | head -n 1 | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' | head -n 1)
  if [[ "${active_version}" != "${REQUIRED_EMSDK_VERSION}" ]]; then
    echo "inditrans requires Emscripten ${REQUIRED_EMSDK_VERSION} (found ${active_version:-unknown})" >&2
    exit 1
  fi
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
    em++ ./native/src/inditrans.cpp -I ./native/src \
      -std=c++23 -g3 --profiling-funcs -s ASSERTIONS=1 -fsanitize=address \
      "-Wl,--no-entry,--export=__wasm_call_ctors" \
      -DDEBUG \
      -s EXPORTED_FUNCTIONS="${exportedFunctions}" \
      -s ENVIRONMENT='web,worker' \
      -s FILESYSTEM=0 \
      -o "${outDir}/inditrans.wasm"
  else
    em++ ./native/src/inditrans.cpp -I ./native/src \
      -std=c++23 -fPIC -Oz -fno-exceptions -fno-rtti -fno-stack-protector -ffunction-sections -fdata-sections -fno-math-errno -DNDEBUG \
      "-Wl,--gc-sections,--no-entry,--export=__wasm_call_ctors" \
      -s EXPORTED_FUNCTIONS='["_malloc", "_free"]' \
      -s STANDALONE_WASM=1 \
      -s ENVIRONMENT='web,worker' \
      -s FILESYSTEM=0 \
      -o "${outDir}/inditrans.wasm"
  fi
}

build_wasm_js() {
  exportedRuntimeMethods='["cwrap", "UTF8ToString"]'
  exportedFunctions='["_malloc", "_free", "_transliterate", "_isScriptSupported", "_releaseBuffer"]'

  # get the path to the output directory
  outDir='./js/public'

  # create the output directory if it does not exist
  if [ ! -d "$outDir" ]; then
    mkdir -p "$outDir"
  fi

  # build the function
  if [ "$1" == "debug" ]; then
    em++ ./native/src/inditrans.cpp -I ./native/src \
      -std=c++23 -g3 --profiling-funcs -s ASSERTIONS=1 -fsanitize=address \
      "-Wl,--no-entry" \
      -DDEBUG \
      -s EXPORTED_FUNCTIONS="$exportedFunctions" \
      -s EXPORTED_RUNTIME_METHODS="$exportedRuntimeMethods" \
      -s WASM=1 \
      -s ENVIRONMENT='web,node' \
      -s SINGLE_FILE=1 \
      -s ALLOW_MEMORY_GROWTH=1 \
      -s EXIT_RUNTIME=0 \
      -s FILESYSTEM=0 \
      --post-js ./js/src/inditrans.post.js \
      -o "$outDir/inditrans.js"
  else
    em++ ./native/src/inditrans.cpp -I ./native/src \
      -std=c++23 -Oz -fno-exceptions -fno-rtti -fno-stack-protector -ffunction-sections -fdata-sections -fno-math-errno \
      "-Wl,--gc-sections,--no-entry" \
      -DNDEBUG \
      -s EXPORTED_FUNCTIONS="$exportedFunctions" \
      -s EXPORTED_RUNTIME_METHODS="$exportedRuntimeMethods" \
      -s WASM=1 \
      -s ENVIRONMENT='web,node' \
      -s SINGLE_FILE=1 \
      -s ALLOW_MEMORY_GROWTH=1 \
      -s EXIT_RUNTIME=0 \
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
