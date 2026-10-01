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
NATIVE_BENCH = out/inditrans_bench$(EXEC_EXT)
NATIVE_SHORT_BENCH = out/inditrans_short_bench$(EXEC_EXT)
NATIVE_MEM_BENCH = out/inditrans_mem_bench$(EXEC_EXT)
NATIVE_ALLOC_BENCH = out/inditrans_alloc_bench$(EXEC_EXT)
LINUX_ALLOC_PROBE = out/libinditrans_alloc_probe.so
ALLOC_BENCH_MODE ?= devanagari
ALLOC_BENCH_BYTES ?= 1048576
ALLOC_BENCH_REPETITIONS ?= 1
ALLOC_BENCH_WARMUPS ?= 0
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

dll: $(NATIVE_CPP) $(NATIVE_H) $(NATIVE_SRC)/CMakeLists.txt tool/cmake/mingw64.cmake
	cmake -S $(NATIVE_SRC) -B $(NATIVE_DIR)/build_win -DCMAKE_TOOLCHAIN_FILE=$(abspath tool/cmake/mingw64.cmake)
	cmake --build $(NATIVE_DIR)/build_win
	cp $(NATIVE_DIR)/build_win/inditrans.dll $(EXAMPLE_DART)/inditrans.dll

windows: dll

dylib:
	echo "macOS cross-compilation requires macOS SDK and osxcross which are not available."

profile:
	g++ -std=c++23 -O1 -fno-exceptions -pg -Wno-normalized -I $(NATIVE_SRC) -I $(NATIVETEST_DIR) $(NATIVE_CPP) $(NATIVETEST_DIR)/test.cpp -o out/prof_$(NATIVE_TEST)
	out/prof_$(NATIVE_TEST) -p
	gprof out/prof_$(NATIVE_TEST) gmon.out > out/native-prof.log

$(NATIVE_TEST): $(NATIVE_CPP) $(NATIVE_H) $(NATIVETEST_CC) $(NATIVETEST_H)
	clang++ -std=c++23 -DBOOST_UT_DISABLE_MODULE -fdiagnostics-color=always -O0 -g -I $(NATIVE_SRC) $(NATIVE_CPP) $(NATIVETEST_CC) -o $@

$(NATIVE_CLI): $(NATIVE_CPP) $(NATIVE_H) $(NATIVE_DIR)/cli/main.cpp
	clang++ -std=c++23 -fdiagnostics-color=always -O0 -g -I $(NATIVE_SRC) $(NATIVE_CPP) $(NATIVE_DIR)/cli/main.cpp -o $@

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
testall: test test_wasm test_flutter test_nodejs

test: $(NATIVE_TEST) test-files/test-cases.json
	$(NATIVE_TEST)

test_wasm: flutter/assets/inditrans.wasm
	node tool/smoke_test_wasm.js

bench: $(NATIVE_BENCH)
	$(NATIVE_BENCH)

.PHONY: bench-short
bench-short: $(NATIVE_SHORT_BENCH)
	$(NATIVE_SHORT_BENCH)

mem-bench: $(NATIVE_MEM_BENCH)
	$(NATIVE_MEM_BENCH)

# Run all benchmarks and print a human-readable summary.
.PHONY: bench-all
bench-all: $(NATIVE_BENCH) $(NATIVE_SHORT_BENCH) $(NATIVE_MEM_BENCH) $(NATIVE_ALLOC_BENCH) $(LINUX_ALLOC_PROBE)
	@echo ""
	@echo "━━━  Throughput — median ns/call and MB/s  (32 B / 4 KiB / 1 MiB inputs)  ━━━"
	@$(NATIVE_BENCH) | awk -F, '\
	  NR==1 { printf "%-28s  %8s  %10s  %8s  %10s\n","case","bytes","median_ns","MB/s","p95_ns" } \
	  NR >1 { mbs=$$2/$$3*1000; printf "%-28s  %8s  %10.0f  %8.1f  %10.0f\n",$$1,$$2,$$3,mbs,$$4 }'
	@echo ""
	@echo "━━━  Short-call latency  (single fixed input, 10 000 samples)  ━━━"
	@$(NATIVE_SHORT_BENCH) | awk -F, '\
	  NR==1 { printf "%-28s  %8s  %9s  %9s\n","case","bytes","p50_µs","p95_µs" } \
	  NR >1 { printf "%-28s  %8s  %9.3f  %9.3f\n",$$1,$$2,$$3/1000,$$4/1000 }'
	@echo ""
	@echo "━━━  Memory footprint  (1 MiB single call, devanagari→telugu)  ━━━"
	@$(NATIVE_MEM_BENCH) | awk -F, '{ printf "  input: %s B   output: %s B\n",$$2,$$3 }'
	@echo ""
	@echo "━━━  Allocations per call  (1 MiB, Linux/glibc only)  ━━━"
	@if [ -f $(LINUX_ALLOC_PROBE) ]; then \
	  printf "%-22s  %8s  %8s  %8s  %8s  %10s  %10s  %10s\n" "mode" "malloc" "realloc" "free" "total" "alloc_B" "peak_B" "live_B"; \
	  for mode in devanagari latin virtual-indic expansion protected mixed-protected; do \
	    raw=$$(LD_PRELOAD=$(LINUX_ALLOC_PROBE) $(NATIVE_ALLOC_BENCH) $$mode 1048576 1 0 2>&1 1>/dev/null); \
	    echo "$$raw" | awk -v m=$$mode '\
	      { \
	        split($$1,a,"="); split(a[2],mc,","); \
	        split($$3,b,"="); split(b[2],rc,","); \
	        split($$4,c,"="); fc=c[2]; \
	        split($$6,d,"="); pk=d[2]; \
	        split($$5,e,"="); lv=e[2]; \
	        allb=mc[2]+0+rc[2]+0; total=mc[1]+rc[1]; \
	        printf "%-22s  %8s  %8s  %8s  %8s  %10s  %10s  %10s\n",m,mc[1],rc[1],fc,total,allb,pk,lv \
	      }'; \
	  done; \
	else \
	  echo "  (skipped: $(LINUX_ALLOC_PROBE) not built — Linux/glibc only)"; \
	fi
	@echo ""

# Linux/glibc-only allocator event profile; does not affect production builds.
.PHONY: bench-alloc-linux
bench-alloc-linux: $(NATIVE_ALLOC_BENCH) $(LINUX_ALLOC_PROBE)
	LD_PRELOAD=$(LINUX_ALLOC_PROBE) $(NATIVE_ALLOC_BENCH) $(ALLOC_BENCH_MODE) $(ALLOC_BENCH_BYTES) $(ALLOC_BENCH_REPETITIONS) $(ALLOC_BENCH_WARMUPS)

$(NATIVE_ALLOC_BENCH) $(LINUX_ALLOC_PROBE): | out

out:
	mkdir -p out

$(NATIVE_BENCH): $(NATIVE_CPP) $(NATIVE_H) $(NATIVE_DIR)/bench/benchmark.cpp
	clang++ -std=c++23 -O3 -DNDEBUG -I $(NATIVE_SRC) $(NATIVE_CPP) $(NATIVE_DIR)/bench/benchmark.cpp -o $@

$(NATIVE_SHORT_BENCH): $(NATIVE_CPP) $(NATIVE_H) $(NATIVE_DIR)/bench/short_call.cpp
	clang++ -std=c++23 -O3 -DNDEBUG -I $(NATIVE_SRC) $(NATIVE_CPP) $(NATIVE_DIR)/bench/short_call.cpp -o $@

$(NATIVE_MEM_BENCH): $(NATIVE_CPP) $(NATIVE_H) $(NATIVE_DIR)/bench/memory.cpp
	clang++ -std=c++23 -O3 -DNDEBUG -I $(NATIVE_SRC) $(NATIVE_CPP) $(NATIVE_DIR)/bench/memory.cpp -o $@

$(NATIVE_ALLOC_BENCH): $(NATIVE_CPP) $(NATIVE_H) $(NATIVE_DIR)/bench/memory.cpp
	clang++ -std=c++23 -O3 -DNDEBUG -DINDTRANSLIT_ALLOC_PROBE -I $(NATIVE_SRC) $(NATIVE_CPP) $(NATIVE_DIR)/bench/memory.cpp -ldl -o $@

$(LINUX_ALLOC_PROBE): $(NATIVE_DIR)/bench/allocation_probe_linux.c
	clang -std=c11 -O2 -fPIC -shared $(NATIVE_DIR)/bench/allocation_probe_linux.c -o $@

test_flutter: wasm flutter/lib/src/bindings.dart
	cd flutter/example && flutter run -d chrome

test_nodejs:
	cd nodejs && yarn && yarn test

cli: $(NATIVE_CLI)
	$(NATIVE_CLI)
