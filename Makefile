default: all

all: native wasm

version:
	dart ./scripts/bump_version.dart

ifeq ($(OS), Windows_NT)
    NATIVE_EXEC=out/inditrans_native.exe
    SCRIPT_EXT=ps1
else
    NATIVE_EXEC=out/inditrans_native
	SCRIPT_EXT=sh
endif
HEADERS_CC = $(wildcard native/src/*.h)
SOURCES_CC = $(wildcard native/src/*.cpp)
SOURCES_NODEJS = $(wildcard nodejs/src/*.ts)
SOURCES_FLUTTER = $(wildcard flutter/lib/src/*.dart)
TEST_CC = $(wildcard native/tests/*.cpp)

# Native
native: $(NATIVE_EXEC)

profile:
	g++ -std=c++20 -O1 -fno-exceptions -pg -Wno-normalized -I native/src $(SOURCES_CC) native/tests/test.cpp -o out/prof_$(NATIVE_EXEC)
	out/prof_$(NATIVE_EXEC) -p
	gprof out/prof_$(NATIVE_EXEC) gmon.out > out/native-prof.log

$(NATIVE_EXEC): $(SOURCES_CC) $(HEADERS_CC) $(TEST_CC)
	clang++ -std=c++20 -g3 -I native/src $(SOURCES_CC) $(TEST_CC) -o $@

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

wasm: flutter/assets/inditrans.wasm js/dist/inditrans.js

flutter/assets/inditrans.wasm: $(SOURCES_CC) $(HEADERS_CC)
	./scripts/build_wasm.$(SCRIPT_EXT) standalone

js/dist/inditrans.js: $(SOURCES_CC) $(HEADERS_CC) js/src/inditrans.post.js
	./scripts/build_wasm.$(SCRIPT_EXT) js

flutter/lib/src/bindings.dart: native/src/exports.h
	dart ./scripts/generate_bindings.dart

native/src/script_data.h: scripts/script_data.json
	dart ./scripts/generate_script_data_header.dart
