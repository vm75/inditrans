#!/bin/bash

self=$(realpath "${0}")
scriptDir=$(dirname "${self}")
sourceRoot=$(dirname "${scriptDir}")
outDir="${sourceRoot}/out"

# native
nativeSourceDir="${sourceRoot}/native"
nativeTarget="${outDir}/native/inditrans_test"

# wasm
wasmSourceDir="${sourceRoot}/wasm"

buildWasm() {
  if ${DEBUG} ; then
    cppoptions="-std=c++20 -g3 --profiling-funcs -s ASSERTIONS=1 -fsanitize=address"
  else
    cppoptions="-std=c++20 -O3 -fno-exceptions -fno-rtti -fno-stack-protector -ffunction-sections -fdata-sections -fno-math-errno -Wl,--gc-sections"
  fi
  docker run --rm -v "${nativeSourceDir}/src:/src" -v "${wasmSourceDir}/src:/src/js" -u $(id -u):$(id -g) \
    emscripten/emsdk emcc inditrans.cpp -o "js/inditrans_wasm.js" ${cppoptions} \
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
    --extern-pre-js js/inditrans_wasm.extern-pre.js \
    --pre-js js/inditrans_wasm.pre.js \
    --post-js js/inditrans_wasm.post.js
}

# emcc -O3 -s MAIN_MODULE=1 -s EXPORT_NAME=libopus -s MODULARIZE=1 ./.libs/libopus.a -o ./emc_out/libopus.js
buildWasmFfi() {
  if ${DEBUG} ; then
    cppoptions="-std=c++20 -g3 --profiling-funcs -s ASSERTIONS=1 -fsanitize=address"
  else
    cppoptions="-std=c++20 -O3 -fno-exceptions -fno-rtti -fno-stack-protector -ffunction-sections -fdata-sections -fno-math-errno -Wl,--gc-sections"
  fi
  docker run --rm -v "${nativeSourceDir}/src:/src" -v "${wasmSourceDir}/src:/src/js" -u $(id -u):$(id -g) \
    emscripten/emsdk emcc inditrans.cpp -o "js/libinditrans.js" ${cppoptions} \
    -s WASM=1 \
    -s MAIN_MODULE=1 \
    -s NO_EXIT_RUNTIME=1 \
    -s EXPORT_NAME=libinditrans \
    -s ENVIRONMENT='web' \
    -s MODULARIZE=1

  em++ ${cppoptions} -o libinditrans.js inditrans.cpp \
    -s EXPORT_NAME=libinditrans \
    -s ENVIRONMENT='web' \
    -s FILESYSTEM=0 \
    -s MAIN_MODULE=1 \
    -s MODULARIZE=1

}

buildNative() {
  mkdir -p "${outDir}/native"
  rm -f "${nativeTarget}"
  if ${DEBUG} ; then
    cppoptions="-std=c++20 -O3 -fno-exceptions"
  else
    cppoptions="-std=c++20 -O3 -fno-exceptions -fno-rtti -fno-stack-protector -ffunction-sections -fdata-sections -fno-math-errno -Wl,--gc-sections"
  fi
  clang++ ${cppoptions} -I "${nativeSourceDir}/src" "${nativeSourceDir}/src"/*.cpp "${nativeSourceDir}/tests"/*.cpp -o "${nativeTarget}"
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
  g++ -std=c++20 -O1 -fno-exceptions -pg -Wno-normalized ${cppoptions} -I "${scriptDir}/src" "${scriptDir}/src"/*.cpp "${scriptDir}/test"/*.cpp -o "${targetBin}"
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
      w2|wasmffi)
        targets="WasmFfi ${targets}"
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
