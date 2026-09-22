default: all

all: native wasm flutter

version:
	dart ./tool/bump_version.dart

validate:
	dart ./tool/verify_release.dart

ifeq ($(OS), Windows_NT)
    EXEC_EXT = .exe
    SCRIPT_EXT = ps1
else
    SCRIPT_EXT = sh
endif
NATIVE_DIR = native
NATIVE_SRC = $(NATIVE_DIR)/src
NATIVE_CLI = out/inditrans$(EXEC_EXT)
NATIVE_TEST = out/inditrans_test$(EXEC_EXT)
NATIVE_CPP = $(wildcard $(NATIVE_SRC)/*.cpp)
NATIVE_H = $(wildcard $(NATIVE_SRC)/*.h)
NATIVETEST_DIR = $(NATIVE_DIR)/tests
NATIVETEST_CC = $(wildcard $(NATIVE_DIR)/tests/*.cpp)
NATIVETEST_H = $(wildcard $(NATIVE_DIR)/tests/*.h)
GENERATOR_UTILS = $(wildcard tool/utils/*.dart)
EXAMPLE_DART = flutter/example.dart

# build
native: $(NATIVE_TEST) $(NATIVE_CLI)

libs: so dll

so: $(NATIVE_CPP) $(NATIVE_H) $(NATIVE_SRC)/CMakeLists.txt
	cmake -S $(NATIVE_SRC) -B $(NATIVE_DIR)/build_linux
	cmake --build $(NATIVE_DIR)/build_linux
	cp $(NATIVE_DIR)/build_linux/libinditrans.so $(EXAMPLE_DART)/libinditrans.so

dll: $(NATIVE_CPP) $(NATIVE_H) $(NATIVE_SRC)/CMakeLists.txt
	cmake -S $(NATIVE_SRC) -B $(NATIVE_DIR)/build_win -DCMAKE_SYSTEM_NAME=Windows -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++
	cmake --build $(NATIVE_DIR)/build_win
	cp $(NATIVE_DIR)/build_win/*inditrans.dll $(EXAMPLE_DART)/inditrans.dll

dylib:
	echo "macOS cross-compilation requires macOS SDK and osxcross which are not available."

profile:
	g++ -std=c++20 -O1 -fno-exceptions -pg -Wno-normalized -I $(NATIVE_SRC) -I $(NATIVETEST_DIR) $(NATIVE_CPP) $(NATIVETEST_DIR)/test.cpp -o out/prof_$(NATIVE_TEST)
	out/prof_$(NATIVE_TEST) -p
	gprof out/prof_$(NATIVE_TEST) gmon.out > out/native-prof.log

$(NATIVE_TEST): $(NATIVE_CPP) $(NATIVE_H) $(NATIVETEST_CC) $(NATIVETEST_H)
	clang++ -std=c++20 -DBOOST_UT_DISABLE_MODULE -fdiagnostics-color=always -O0 -g -I $(NATIVE_SRC) $(NATIVE_CPP) $(NATIVETEST_CC) -o $@

$(NATIVE_CLI): $(NATIVE_CPP) $(NATIVE_H) $(NATIVE_DIR)/cli/main.cpp
	clang++ -std=c++20 -fdiagnostics-color=always -O0 -g -I $(NATIVE_SRC) $(NATIVE_CPP) $(NATIVE_DIR)/cli/main.cpp -o $@

$(NATIVE_SRC)/script_data.h: tool/script_data.json tool/options.json tool/generate_headers.dart $(GENERATOR_UTILS)
	dart tool/generate_headers.dart

wasm: flutter/assets/inditrans.wasm js/public/inditrans.js

flutter/assets/inditrans.wasm: $(NATIVE_CPP) $(NATIVE_H)
	./tool/build_wasm.$(SCRIPT_EXT) standalone

js/public/inditrans.js: $(NATIVE_CPP) $(NATIVE_H) js/src/inditrans.post.js
	./tool/build_wasm.$(SCRIPT_EXT) js

flutter/lib/src/bindings.dart: $(NATIVE_SRC)/exports.h
	dart tool/generate_bindings.dart

flutter: flutter/lib/src/bindings.dart flutter/assets/inditrans.wasm

# test
testall: test test_flutter test_nodejs

test: $(NATIVE_TEST) test-files/test-cases.json
	$(NATIVE_TEST)

test_flutter: wasm flutter/lib/src/bindings.dart
	cd flutter/example && flutter run -d chrome

test_nodejs:
	cd nodejs && yarn && yarn test

cli: $(NATIVE_CLI)
	$(NATIVE_CLI)

# publish (manual fallback; canonical releases are tag-triggered in GitHub Actions)
publish: publish_flutter publish_nodejs

publish_flutter:
	cd flutter && flutter pub publish

publish_nodejs:
	cd nodejs && npm publish --access=public
