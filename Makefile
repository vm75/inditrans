default: all

all: native wasm

# Common
ifdef  DEBUG
    compilerOptions=-std=c++20 -g3 --profiling-funcs -s ASSERTIONS=1 -fsanitize=address
    linkerOptions=-Wl,--no-entry
else
    compilerOptions=-std=c++20 -O3 -fno-exceptions -fno-rtti -fno-stack-protector -ffunction-sections -fdata-sections -fno-math-errno
    linkerOptions=-Wl,--gc-sections,--no-entry
endif

ifeq ($(OS), Windows_NT)
    NATIVE_EXEC=out/inditrans_native.exe
else
	NATIVE_EXEC=out/inditrans_native
endif
HEADERS_CC = $(wildcard native/src/*.h)
SOURCES_CC = $(wildcard native/src/*.cpp)
SOURCES_JS = $(wildcard nodejs/wasm/src/*.js)

# Native
native: $(NATIVE_EXEC)

$(NATIVE_EXEC): $(SOURCES_CC) $(HEADERS_CC) native/tests/test.cpp
	-mkdir out
	clang++ ${compilerOptions} -I native/src $(SOURCES_CC) native/tests/test.cpp -o $@ && \
		llvm-strip -s -R .comment -R .gnu.version --strip-unneeded $@

flutter: flutter/assets/inditrans.wasm
	cd flutter/example && flutter build -d 1

nodejs: nodejs/dist/inditrans.mjs
	cd nodejs && tsc

testall: test_native test_flutter test_nodejs

test_native: $(NATIVE_EXEC)
	$(NATIVE_EXEC)

test_flutter:

test_nodejs:

publishall: publish_flutter publish_nodejs

publish_flutter:

publish_nodejs:

# Wasm
runtimeExports="EXPORTED_RUNTIME_METHODS=[\"cwrap\", \"ccall\"]"
compiledExports="EXPORTED_FUNCTIONS=[\"_malloc\", \"_free\", \"_translitOptionsToInt\", \"_transliterate\", \"_transliterate2\", \"_releaseBuffer\"]"

wasm: nodejs/dist/inditrans.mjs flutter/assets/inditrans.wasm

nodejs/dist/inditrans.mjs: $(SOURCES_CC) $(HEADERS_CC) $(SOURCES_JS)
	docker run --rm -v "$(CURDIR)/native/src:/src" -v "$(CURDIR)/nodejs/wasm/src:/src/js" -v "$(CURDIR)/nodejs/dist:/dist" \
		emscripten/emsdk \
			emcc inditrans.cpp -o /dist/inditrans.mjs $(compilerOptions) $(linkerOptions) -flto \
				-s WASM=1 \
				-s ENVIRONMENT='web,node' \
				-s NO_EXIT_RUNTIME=1 \
				-s MODULARIZE=1 \
				-s EXPORT_NAME=inditrans \
				-s FILESYSTEM=0 \
				-s SINGLE_FILE=1 \
				-s ALLOW_MEMORY_GROWTH=1 \
				-s $(runtimeExports) \
				-s $(compiledExports) \
				--extern-pre-js /src/js/inditrans.extern-pre.js \
				--pre-js /src/js/inditrans.pre.js \
				--post-js /src/js/inditrans.post.js

flutter/assets/inditrans.wasm: $(SOURCES_CC) $(HEADERS_CC)
	docker run --rm -v "$(CURDIR)/native/src:/src" -v "$(CURDIR)/flutter/assets:/dist" \
		emscripten/emsdk \
			emcc inditrans.cpp -o /dist/inditrans.wasm $(compilerOptions) $(linkerOptions) -flto \
				-DNDEBUG \
				-s EXPORT_NAME=inditrans \
				-s FILESYSTEM=0 \
				-s $(compiledExports)
