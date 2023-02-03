default: all

all: native wasm

# Common
ifdef  DEBUG
    COMPILER_OPTIONS=-std=c++20 -g3 --profiling-funcs -s ASSERTIONS=1 -fsanitize=address
    LINKER_OPTIONS=-Wl,--no-entry
else
    COMPILER_OPTIONS=-std=c++20 -O3 -fno-exceptions -fno-rtti -fno-stack-protector -ffunction-sections -fdata-sections -fno-math-errno
    LINKER_OPTIONS=-Wl,--gc-sections,--no-entry
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
	clang++ ${COMPILER_OPTIONS} -I native/src $(SOURCES_CC) native/tests/test.cpp -o $@ && \
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
RUNTIME_EXPORTS="EXPORTED_RUNTIME_METHODS=[\"cwrap\", \"ccall\"]"
COMPILED_EXPORTS="EXPORTED_FUNCTIONS=[\"_malloc\", \"_free\", \"_translitOptionsToInt\", \"_transliterate\", \"_transliterate2\", \"_releaseBuffer\"]"

NODEJS_TARGET=wasm/dist/inditrans.mjs
FLUTTER_TARGET=assets/inditrans.wasm

wasm: nodejs/$(NODEJS_TARGET) flutter/$(FLUTTER_TARGET)

nodejs/$(NODEJS_TARGET): $(SOURCES_CC) $(HEADERS_CC) $(SOURCES_JS)
	docker run --rm -v "$(CURDIR)/native/src:/src" -v "$(CURDIR)/nodejs/wasm/src:/src/js" -v "$(CURDIR)/nodejs:/dist" \
		emscripten/emsdk \
			emcc inditrans.cpp -o /dist/$(NODEJS_TARGET) $(COMPILER_OPTIONS) $(LINKER_OPTIONS) -flto \
				-s WASM=1 \
				-s ENVIRONMENT='web,node' \
				-s NO_EXIT_RUNTIME=1 \
				-s MODULARIZE=1 \
				-s EXPORT_NAME=inditrans \
				-s FILESYSTEM=0 \
				-s SINGLE_FILE=1 \
				-s ALLOW_MEMORY_GROWTH=1 \
				-s $(RUNTIME_EXPORTS) \
				-s $(COMPILED_EXPORTS) \
				--extern-pre-js /src/js/inditrans.extern-pre.js \
				--pre-js /src/js/inditrans.pre.js \
				--post-js /src/js/inditrans.post.js

flutter/$(FLUTTER_TARGET): $(SOURCES_CC) $(HEADERS_CC)
	docker run --rm -v "$(CURDIR)/native/src:/src" -v "$(CURDIR)/flutter:/dist" \
		emscripten/emsdk \
			emcc inditrans.cpp -o /dist/$(FLUTTER_TARGET) $(COMPILER_OPTIONS) $(LINKER_OPTIONS) -flto \
				-DNDEBUG \
				-s EXPORT_NAME=inditrans \
				-s FILESYSTEM=0 \
				-s $(COMPILED_EXPORTS)
