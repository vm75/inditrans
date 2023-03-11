#!/bin/bash

self=$(realpath "${0}")
scriptDir=$(dirname "${self}")
sourceRoot=$(dirname "${scriptDir}")
outDir="${sourceRoot}/out"

# native
nativeSourceDir="${sourceRoot}/native"
nativeTarget="${outDir}/native/inditrans_test"

# wasm
wasmDir="${sourceRoot}/wasm"

buildWasm() {
  if ${DEBUG} ; then
    compilerOptions="-std=c++20 -g3 --profiling-funcs -s ASSERTIONS=1 -fsanitize=address"
    linkerOptions="-Wl,--no-entry"
  else
    compilerOptions="-std=c++20 -O3 -fno-exceptions -fno-rtti -fno-stack-protector -ffunction-sections -fdata-sections -fno-math-errno -flto"
    linkerOptions="-Wl,--gc-sections,--no-entry"
  fi

  runtimeExports='EXPORTED_RUNTIME_METHODS=["cwrap", "ccall"]'
  compiledExports='EXPORTED_FUNCTIONS=["_malloc", "_free", "_translitOptionsToInt", "_transliterate", "_transliterate2", "_releaseBuffer"]'

  docker run --rm -v "${nativeSourceDir}/src:/src" -v "${wasmDir}:/wasm" -u $(id -u):$(id -g) \
    emscripten/emsdk \
      emcc inditrans.cpp -o /wasm/dist/inditrans.mjs ${compilerOptions} ${linkerOptions} \
        -s WASM=1 \
        -s ENVIRONMENT='web,node' \
        -s NO_EXIT_RUNTIME=1 \
        -s MODULARIZE=1 \
        -s EXPORT_NAME=inditrans \
        -s FILESYSTEM=0 \
        -s SINGLE_FILE=1 \
        -s ALLOW_MEMORY_GROWTH=1 \
        -s "${runtimeExports}" \
        -s "${compiledExports}" \
        --extern-pre-js /wasm/src/inditrans.extern-pre.js \
        --pre-js /wasm/src/inditrans.pre.js \
        --post-js /wasm/src/inditrans.post.js

  docker run --rm -v "${nativeSourceDir}/src:/src" -v "${wasmDir}:/wasm" -u $(id -u):$(id -g) \
    emscripten/emsdk \
      emcc inditrans.cpp -o /wasm/dist/inditrans.wasm ${compilerOptions} ${linkerOptions} \
        -DNDEBUG \
        -s EXPORT_NAME=inditrans \
        -s FILESYSTEM=0 \
        -s "${compiledExports}"

  # docker run --rm -v "${nativeSourceDir}/src:/src" -v "${wasmDir}:/wasm" -u $(id -u):$(id -g) \
  #   emscripten/emsdk \
  #     emcc inditrans.cpp -o /wasm/split/inditrans.js ${compilerOptions} ${linkerOptions} \
  #       -s MODULARIZE=1 \
  #       -s EXPORT_NAME=inditrans \
  #       -s NO_EXIT_RUNTIME=1 \
  #       -s FILESYSTEM=0 \
  #       -s ALLOW_MEMORY_GROWTH=1 \
  #       -s "${runtimeExports}" \
  #       -s "${compiledExports}"

  # docker run --rm -v "${nativeSourceDir}/src:/src" -v "${wasmDir}:/wasm" -u $(id -u):$(id -g) \
  #   emscripten/emsdk \
  #     emcc inditrans.cpp -o /wasm/embedded/inditrans.js ${compilerOptions} ${linkerOptions} \
  #       -s WASM=1 \
  #       -s NO_EXIT_RUNTIME=1 \
  #       -s MODULARIZE=1 \
  #       -s EXPORT_NAME=inditrans \
  #       -s FILESYSTEM=0 \
  #       -s SINGLE_FILE=1 \
  #       -s ALLOW_MEMORY_GROWTH=1 \
  #       -s "${runtimeExports}" \
  #       -s "${compiledExports}"
}

buildNative() {
  mkdir -p "${outDir}/native"
  rm -f "${nativeTarget}"
  if ${DEBUG} ; then
    compilerOptions="-std=c++20 -O3 -fno-exceptions"
  else
    compilerOptions="-std=c++20 -O3 -fno-exceptions -fno-rtti -fno-stack-protector -ffunction-sections -fdata-sections -fno-math-errno"
  fi
  clang++ ${compilerOptions} -I "${nativeSourceDir}/src" "${nativeSourceDir}/src"/*.cpp "${nativeSourceDir}/tests"/*.cpp -o "${nativeTarget}"
  if [[ -f "${nativeTarget}" ]] ; then
    strip -s -R .comment -R .gnu.version --strip-unneeded "${nativeTarget}"
  fi
}

testNative() {
  [[ -f ${nativeTarget} ]] && ${nativeTarget}
}

testWasm() {
  cd ts
  npm run build-wasm && tsc && node --no-warnings --experimental-modules --es-module-specifier-resolution=node dist/wasm/test/test.js
  cd -
}

profileNative() {
  g++ -std=c++20 -O1 -fno-exceptions -pg -Wno-normalized ${compilerOptions} -I "${scriptDir}/src" "${scriptDir}/src"/*.cpp "${scriptDir}/test"/*.cpp -o "${targetBin}"
  "${targetBin}" -p
  gprof "${targetBin}" gmon.out > "${distDir}/wasm-prof.log"
}

profileTs() {
  workspaceDir=$(realpath "${scriptDir}/../..")
  distDir=$(realpath "${scriptDir}/dist")

  cd "${workspaceDir}"
  rm isolate*.log
  tsc -p tsconfig.json
  node --prof --no-turbo-inlining --no-warnings --experimental-modules --es-module-specifier-resolution=node dist/ts/test/tests.js
  node --prof-process isolate-*.log > dist/ts-prof.log
}

DEBUG=false
while getopts "b:dp:t:" option; do
  command=
  commandArg=
  case "${option}" in
    d)
      DEBUG=true
      ;;
    b)
      command=build
      ;;
    t)
      command=test
      ;;
    t)
      command=profile
      ;;
  esac
  if [[ -n ${command} ]] ; then
    commandTargets=${command}Targets
    eval "targets=\$${commandTargets}"
    case ${OPTARG} in
      a|all)
        targets="Native Ts Wasm"
        ;;
      n|native)
        targets="Native ${targets}"
        ;;
      t|ts)
        targets="Ts ${targets}"
        ;;
      w|wasm)
        targets="Wasm ${targets}"
        ;;
    esac
    eval "${commandTargets}=${targets}"
  fi
done
shift $((OPTIND-1))

for target in ${buildTargets} ; do
  if [[ $(type -t build${target}) == function ]] ; then
    build${target}
  fi
done

for target in ${testTargets} ; do
  if [[ $(type -t test${target}) == function ]] ; then
    test${target}
  fi
done

emcc -g .\inditrans.cpp  -o inditrans.wasm -std=c++20 "-Wl,--no-entry" -s "EXPORT_NAME=inditrans" -s "FILESYSTEM=0"  -s "EXPORTED_FUNCTIONS=[""_malloc"", ""_free"", ""_transliterate"", ""_releaseBuffer""]"
@REM emcc .\inditrans.cpp  -o inditrans.wasm -std=c++20 -Oz -fno-exceptions -fno-rtti "-Wl,--gc-sections,--no-entry" -s "EXPORT_NAME=inditrans" -s "FILESYSTEM=0"  -s "EXPORTED_FUNCTIONS=[""_malloc"", ""_free"", ""_transliterate"", ""_releaseBuffer""]"

emcc -g .\inditrans.cpp -o inditrans.html -std=c++20 "-Wl,--gc-sections" -s ALLOW_MEMORY_GROWTH=1