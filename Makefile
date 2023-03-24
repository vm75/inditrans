default: all

all: native wasm flutter

version:
	dart ./scripts/bump_version.dart

ifeq ($(OS), Windows_NT)
    EXEC_EXT = .exe
    SCRIPT_EXT = ps1
else
    SCRIPT_EXT = sh
endif
NATIVE_CLI = out/inditrans$(EXEC_EXT)
NATIVE_TEST = out/inditrans_test$(EXEC_EXT)
HEADERS_CC = $(wildcard native/src/*.h)
SOURCES_CC = $(wildcard native/src/*.cpp)
SOURCES_NODEJS = $(wildcard nodejs/src/*.ts)
SOURCES_FLUTTER = $(wildcard flutter/lib/src/*.dart)
TEST_CC = $(wildcard native/tests/*.cpp)
GENERATOR_UTILS = $(wildcard scripts/utils/*.dart)

# build
native: $(NATIVE_TEST) $(NATIVE_CLI)

profile:
	g++ -std=c++20 -O1 -fno-exceptions -pg -Wno-normalized -I native/src $(SOURCES_CC) native/tests/test.cpp -o out/prof_$(NATIVE_TEST)
	out/prof_$(NATIVE_TEST) -p
	gprof out/prof_$(NATIVE_TEST) gmon.out > out/native-prof.log

$(NATIVE_TEST): $(SOURCES_CC) $(HEADERS_CC) $(TEST_CC)
	clang++ -std=c++20 -fdiagnostics-color=always -O0 -g -I native/src $(SOURCES_CC) $(TEST_CC) -o $@

$(NATIVE_CLI): $(SOURCES_CC) $(HEADERS_CC) native/cli/main.cpp
	clang++ -std=c++20 -fdiagnostics-color=always -O0 -g -I native/src $(SOURCES_CC) native/cli/main.cpp -o $@

native/src/script_data.h: headers

headers: scripts/script_data.json scripts/generate_headers.dart $(GENERATOR_UTILS)
	dart scripts/generate_headers.dart

wasm: flutter/assets/inditrans.wasm js/public/inditrans.js

flutter/assets/inditrans.wasm: $(SOURCES_CC) $(HEADERS_CC)
	./scripts/build_wasm.$(SCRIPT_EXT) standalone

js/public/inditrans.js: $(SOURCES_CC) $(HEADERS_CC) js/src/inditrans.post.js
	./scripts/build_wasm.$(SCRIPT_EXT) js

flutter/lib/src/bindings.dart: native/src/exports.h
	dart scripts/generate_bindings.dart

flutter: flutter/lib/src/bindings.dart flutter/assets/inditrans.wasm

# test
testall: test test_flutter test_nodejs

test: $(NATIVE_TEST) test-files/test-cases.json
	$(NATIVE_TEST)

test_flutter: wasm flutter/lib/src/bindings.dart
	cd flutter/example && flutter run -d chrome

test_nodejs:
	cd nodejs && yarn test

cli: $(NATIVE_CLI)
	$(NATIVE_CLI)

# publish
publish: publish_flutter publish_nodejs

publish_flutter:
	cd flutter && flutter pub publish

publish_nodejs:
	cd nodejs && npm publish --access=public
