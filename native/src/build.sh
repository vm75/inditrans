docker run --rm -v "${PWD}:/src" -u $(id -u):$(id -g) \
  emscripten/emsdk emcc inditrans.cpp -Os -o inditrans.wasm -fno-exceptions -std=c++20 -Wl,--no-entry \
    -s WASM=1 -s EXPORTED_FUNCTIONS='["_transliterate"]'