emcc -g .\inditrans.cpp  -o inditrans.wasm -std=c++20 "-Wl,--no-entry" -s "EXPORT_NAME=inditrans" -s "FILESYSTEM=0"  -s "EXPORTED_FUNCTIONS=[""_malloc"", ""_free"", ""_transliterate"", ""_releaseBuffer""]"
@REM emcc .\inditrans.cpp  -o inditrans.wasm -std=c++20 -Oz -fno-exceptions -fno-rtti "-Wl,--gc-sections,--no-entry" -s "EXPORT_NAME=inditrans" -s "FILESYSTEM=0"  -s "EXPORTED_FUNCTIONS=[""_malloc"", ""_free"", ""_transliterate"", ""_releaseBuffer""]"

emcc -g .\inditrans.cpp -o inditrans.html -std=c++20 "-Wl,--gc-sections" -s ALLOW_MEMORY_GROWTH=1