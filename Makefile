default: all

all: native wasm

version:
	dart ./scripts/bump_version.dart

# Common
ifdef DEBUG
    COMPILER_OPTIONS=-std=c++20 -g3 --profiling-funcs -s ASSERTIONS=1 -fsanitize=address
    LINKER_OPTIONS=-Wl,--no-entry
else
    COMPILER_OPTIONS=-std=c++20 -Oz -fno-exceptions -fno-rtti -fno-stack-protector -ffunction-sections -fdata-sections -fno-math-errno
    LINKER_OPTIONS=-Wl,--gc-sections,--no-entry
endif

ifeq ($(OS), Windows_NT)
    NATIVE_EXEC=out/inditrans_native.exe
else
    NATIVE_EXEC=out/inditrans_native
endif
HEADERS_CC = $(wildcard native/src/*.h)
SOURCES_CC = $(wildcard native/src/*.cpp)
SOURCES_NODEJS = $(wildcard nodejs/src/*.ts)
SOURCES_FLUTTER = $(wildcard flutter/lib/src/*.dart)

# Native
native: $(NATIVE_EXEC)

$(NATIVE_EXEC): $(SOURCES_CC) $(HEADERS_CC) native/tests/test.cpp
	clang++ ${COMPILER_OPTIONS} -I native/src $(SOURCES_CC) native/tests/test.cpp -o $@ && \
		llvm-strip -s -R .comment -R .gnu.version --strip-unneeded $@

testall: test test_flutter test_nodejs

test: $(NATIVE_EXEC)
	$(NATIVE_EXEC)

test_flutter: wasm flutter/lib/src/bindings.dart
	cd flutter/example && flutter run -d chrome

test_nodejs:
	cd nodejs && yarn test

publish: publish_flutter publish_nodejs

publish_flutter:
	cd flutter && flutter pub publish

publish_nodejs:
	cd nodejs && npm publish --access=public

# Wasm defines
RUNTIME_EXPORTS="EXPORTED_RUNTIME_METHODS=[\"cwrap\", \"ccall\"]"
COMPILED_EXPORTS="EXPORTED_FUNCTIONS=[\"_malloc\", \"_free\", \"_transliterate\", \"_releaseBuffer\"]"

ifneq ($(OS), Windows_NT)
	USER_SPEC=-u $(shell id -u):$(shell id -g)
else
	USER_SPEC=
endif

WASM_TARGET=assets/inditrans.wasm
wasm: flutter/$(WASM_TARGET)
flutter/$(WASM_TARGET): $(SOURCES_CC) $(HEADERS_CC)
	docker run --rm $(USER_SPEC) -v "$(CURDIR)/native/src:/src" -v "$(CURDIR)/flutter:/dist" \
		emscripten/emsdk \
			emcc inditrans.cpp -o /dist/$(WASM_TARGET) \
				$(COMPILER_OPTIONS) $(LINKER_OPTIONS) \
				-DNDEBUG \
				-s EXPORT_NAME=inditrans \
				-s FILESYSTEM=0 \
				-s $(COMPILED_EXPORTS)

flutter/lib/src/bindings.dart: flutter/native/src/exports.h
	dart ./scripts/generate_bindings.dart
