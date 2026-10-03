default: all

all: native wasm flutter

version:
	python3 ./tool/bump_version.py

validate:
	python3 ./tool/verify_release.py

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
NATIVE_COLD_BENCH = out/inditrans_cold_bench$(EXEC_EXT)
NATIVE_LOOKUP_BENCH = out/inditrans_lookup_bench$(EXEC_EXT)
NATIVE_LOOKUP_ALLOC_BENCH = out/inditrans_lookup_alloc_bench$(EXEC_EXT)
NATIVE_MEM_BENCH = out/inditrans_mem_bench$(EXEC_EXT)
NATIVE_ALLOC_BENCH = out/inditrans_alloc_bench$(EXEC_EXT)
LINUX_ALLOC_PROBE = out/libinditrans_alloc_probe.so
ALLOC_BENCH_MODE ?= devanagari
ALLOC_BENCH_BYTES ?= 1048576
ALLOC_BENCH_REPETITIONS ?= 1
ALLOC_BENCH_WARMUPS ?= 0
BENCH_BASELINE ?= out/bench-baseline
BENCH_REGRESSION_THRESHOLD ?= 5
BENCH_STRICT ?= 0
BENCH_RUNS ?= 5
BENCH_CPU ?= $(PERF_CPU)
PERF_DIR ?= out/perf
PERF_NAME ?=
PERF_BASELINE ?= 00-baseline
PERF_LATENCY_CASE ?= indic-to-indic
PERF_ALLOC_CASE ?= devanagari
PERF_COLD_CASE ?= cold-devanagari-to-telugu
PERF_REPEATS ?= 5
PERF_CPU ?=
COLD_BENCH_SAMPLES ?= 501
PERF_TOOL_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))tool
ALLOC_PROBE_SUPPORTED := $(shell test "$$(uname -s 2>/dev/null)" = "Linux" && getconf GNU_LIBC_VERSION >/dev/null 2>&1 && echo 1 || echo 0)
ifeq ($(ALLOC_PROBE_SUPPORTED),1)
    ALLOC_BENCH_DEPS = $(NATIVE_ALLOC_BENCH) $(LINUX_ALLOC_PROBE)
else
    ALLOC_BENCH_DEPS =
endif
NATIVE_CPP = $(wildcard $(NATIVE_SRC)/*.cpp)
NATIVE_H = $(wildcard $(NATIVE_SRC)/*.h)
NATIVETEST_DIR = $(NATIVE_DIR)/tests
NATIVETEST_CC = $(wildcard $(NATIVE_DIR)/tests/*.cpp)
NATIVETEST_H = $(wildcard $(NATIVE_DIR)/tests/*.h)
GENERATOR_UTILS = $(wildcard tool/python/*.py)
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

$(NATIVE_SRC)/script_data.h: tool/script_data.json tool/reader_data.json tool/options.json docs/extended-latin.txt tool/generate_headers.py $(GENERATOR_UTILS)
	python3 tool/generate_headers.py

wasm: flutter/assets/inditrans.wasm js/public/inditrans.js

flutter/assets/inditrans.wasm: $(NATIVE_CPP) $(NATIVE_H)
	./tool/build_wasm.$(SCRIPT_EXT) standalone

js/public/inditrans.js: $(NATIVE_CPP) $(NATIVE_H) js/src/inditrans.post.js
	./tool/build_wasm.$(SCRIPT_EXT) js

flutter/lib/src/bindings.dart: $(NATIVE_SRC)/exports.h tool/generate_bindings.py
	python3 tool/generate_bindings.py

flutter: flutter/lib/src/bindings.dart flutter/assets/inditrans.wasm

# test
testall: test test_wasm test_flutter test_nodejs

test: $(NATIVE_TEST) test-files/test-cases.json
	$(NATIVE_TEST)

test_wasm: flutter/assets/inditrans.wasm
	node tool/smoke_test_wasm.js

bench: $(NATIVE_BENCH)
	$(NATIVE_BENCH)

.PHONY: bench-lookup bench-lookup-alloc-linux
bench-lookup: $(NATIVE_LOOKUP_BENCH)
	$(NATIVE_LOOKUP_BENCH)

ifeq ($(ALLOC_PROBE_SUPPORTED),1)
bench-lookup-alloc-linux: $(NATIVE_LOOKUP_ALLOC_BENCH) $(LINUX_ALLOC_PROBE)
	LD_PRELOAD=$(LINUX_ALLOC_PROBE) $(NATIVE_LOOKUP_ALLOC_BENCH) --alloc
else
bench-lookup-alloc-linux:
	@echo "bench-lookup-alloc-linux requires Linux with glibc."
	@exit 2
endif

.PHONY: bench-short
bench-short: $(NATIVE_SHORT_BENCH)
	$(NATIVE_SHORT_BENCH)

.PHONY: bench-short-repeat
bench-short-repeat: $(NATIVE_SHORT_BENCH)
	@python3 "$(PERF_TOOL_DIR)/repeat_short_bench.py" --binary $(NATIVE_SHORT_BENCH) --runs $(PERF_REPEATS) --cpu "$(PERF_CPU)"

.PHONY: bench-cold
bench-cold: $(NATIVE_COLD_BENCH)
	@python3 "$(PERF_TOOL_DIR)/cold_start_bench.py" --binary $(NATIVE_COLD_BENCH) --samples $(COLD_BENCH_SAMPLES) --cpu "$(PERF_CPU)"

.PHONY: output-size-bench mem-bench
output-size-bench: $(NATIVE_MEM_BENCH)
	$(NATIVE_MEM_BENCH)

# Backward-compatible alias. This reports output expansion, not heap usage.
mem-bench: output-size-bench

# Run all benchmarks and print a human-readable summary.
.PHONY: bench-all
bench-all: $(NATIVE_BENCH) $(NATIVE_SHORT_BENCH) $(NATIVE_COLD_BENCH) $(NATIVE_MEM_BENCH) $(ALLOC_BENCH_DEPS)
	@echo ""
	@echo "━━━  Throughput — median ns/call and MB/s  (~32 B / ~4 KiB / ~1 MiB targets; actual bytes shown)  ━━━"
	@$(NATIVE_BENCH) | awk -F, '\
	  NR==1 { printf "%-28s  %8s  %10s  %8s  %10s\n","case","bytes","median_ns","MB/s","p95_ns" } \
	  NR >1 { mbs=$$2/$$3*1000; printf "%-28s  %8s  %10.0f  %8.1f  %10.0f\n",$$1,$$2,$$3,mbs,$$4 }'
	@echo ""
	@echo "━━━  Short-call latency  (single fixed input, 10 000 samples; includes result destruction)  ━━━"
	@$(NATIVE_SHORT_BENCH) | awk -F, '\
	  NR==1 { printf "%-28s  %8s  %9s  %9s\n","case","bytes","p50_µs","p95_µs" } \
	  NR >1 { printf "%-28s  %8s  %9.3f  %9.3f\n",$$1,$$2,$$3/1000,$$4/1000 }'
	@echo ""
	@echo "━━━  Cold-start latency  (first transliteration in fresh processes; $(COLD_BENCH_SAMPLES) samples)  ━━━"
	@python3 "$(PERF_TOOL_DIR)/cold_start_bench.py" --binary $(NATIVE_COLD_BENCH) --samples $(COLD_BENCH_SAMPLES) --cpu "$(PERF_CPU)" | awk -F, '\
	  NR==1 { printf "%-30s %8s %8s %9s %9s %s\n","case","bytes","samples","p50_µs","p95_µs","output_fnv1a64" } \
	  NR>1 { printf "%-30s %8s %8s %9.3f %9.3f %s\n",$$1,$$2,$$5,$$3/1000,$$4/1000,$$7 }'
	@echo ""
	@echo "━━━  Output size / expansion  (~1 MiB target, devanagari→telugu)  ━━━"
	@$(NATIVE_MEM_BENCH) | awk -F, '{ printf "  input: %s B   output: %s B   expansion: %.3fx\n",$$2,$$3,$$3/$$2 }'
	@echo ""
	@echo "━━━  Cold allocations  (~1 MiB target; first measured transliteration; Linux/glibc only)  ━━━"
	@if [ "$(ALLOC_PROBE_SUPPORTED)" = "1" ] && [ -f $(LINUX_ALLOC_PROBE) ]; then \
	  python3 "$(PERF_TOOL_DIR)/allocation_bench.py" --binary $(NATIVE_ALLOC_BENCH) --probe $(LINUX_ALLOC_PROBE) --warmups 0 --format table; \
	else \
	  echo "  (skipped: allocator probe requires Linux with glibc)"; \
	fi
	@echo ""
	@echo "━━━  Warm allocations  (~1 MiB target; one unmeasured warmup; Linux/glibc only)  ━━━"
	@if [ "$(ALLOC_PROBE_SUPPORTED)" = "1" ] && [ -f $(LINUX_ALLOC_PROBE) ]; then \
	  python3 "$(PERF_TOOL_DIR)/allocation_bench.py" --binary $(NATIVE_ALLOC_BENCH) --probe $(LINUX_ALLOC_PROBE) --warmups 1 --format table; \
	else \
	  echo "  (skipped: allocator probe requires Linux with glibc)"; \
	fi
	@echo ""

# Save benchmark results as a baseline for later comparison with bench-compare.
.PHONY: bench-save
bench-save: $(NATIVE_BENCH) $(NATIVE_SHORT_BENCH) $(ALLOC_BENCH_DEPS) | out
	@mkdir -p "$(dir $(BENCH_BASELINE))"
	@python3 "$(PERF_TOOL_DIR)/repeat_short_bench.py" --binary $(NATIVE_BENCH) --runs $(BENCH_RUNS) --cpu "$(BENCH_CPU)" --raw-output "$(BENCH_BASELINE)-throughput-runs.csv" > $(BENCH_BASELINE)-throughput.csv
	@python3 "$(PERF_TOOL_DIR)/repeat_short_bench.py" --binary $(NATIVE_SHORT_BENCH) --runs $(BENCH_RUNS) --cpu "$(BENCH_CPU)" --raw-output "$(BENCH_BASELINE)-latency-runs.csv" > $(BENCH_BASELINE)-latency.csv
	@printf "" > $(BENCH_BASELINE)-allocs.csv
	@printf "" > $(BENCH_BASELINE)-allocs-warm.csv
	@if [ "$(ALLOC_PROBE_SUPPORTED)" = "1" ] && [ -f $(LINUX_ALLOC_PROBE) ]; then \
	  python3 "$(PERF_TOOL_DIR)/allocation_bench.py" --binary $(NATIVE_ALLOC_BENCH) --probe $(LINUX_ALLOC_PROBE) --warmups 0 --format csv > $(BENCH_BASELINE)-allocs.csv; \
	  python3 "$(PERF_TOOL_DIR)/allocation_bench.py" --binary $(NATIVE_ALLOC_BENCH) --probe $(LINUX_ALLOC_PROBE) --warmups 1 --format csv > $(BENCH_BASELINE)-allocs-warm.csv; \
	fi
	@python3 "$(PERF_TOOL_DIR)/perf_metadata.py" > $(BENCH_BASELINE)-info.txt
	@printf "bench_runs=%s\nbench_cpu=%s\n" "$(BENCH_RUNS)" "$(BENCH_CPU)" >> $(BENCH_BASELINE)-info.txt
	@echo "Baseline saved → averaged CSVs plus raw *-runs.csv ($(BENCH_RUNS) runs)"
	@cat $(BENCH_BASELINE)-info.txt

# Save one cumulative tuning snapshot using the benchmark formats plus raw standalone Wasm size.
.PHONY: perf-snapshot perf-report
perf-snapshot: $(NATIVE_COLD_BENCH) | out
	@if [ -z "$(PERF_NAME)" ]; then echo "Set PERF_NAME, for example: make perf-snapshot PERF_NAME=00-baseline"; exit 2; fi
	@mkdir -p "$(PERF_DIR)"
	@$(MAKE) bench-save BENCH_BASELINE="$(PERF_DIR)/$(PERF_NAME)"
	@python3 "$(PERF_TOOL_DIR)/cold_start_bench.py" --binary $(NATIVE_COLD_BENCH) --samples $(COLD_BENCH_SAMPLES) --cpu "$(PERF_CPU)" > "$(PERF_DIR)/$(PERF_NAME)-cold.csv"
	@$(MAKE) -B flutter/assets/inditrans.wasm
	@python3 -c "from pathlib import Path; print(Path('flutter/assets/inditrans.wasm').stat().st_size)" > "$(PERF_DIR)/$(PERF_NAME)-wasm-size.txt"
	@echo "Wasm size saved → $(PERF_DIR)/$(PERF_NAME)-wasm-size.txt"

perf-report:
	@python3 "$(PERF_TOOL_DIR)/perf_report.py" --dir "$(PERF_DIR)" --baseline "$(PERF_BASELINE)" --latency-case "$(PERF_LATENCY_CASE)" --alloc-case "$(PERF_ALLOC_CASE)" --cold-case "$(PERF_COLD_CASE)"

# Compare current results against the saved baseline. Output mismatches always fail.
# Performance regressions are reported by default and fail when BENCH_STRICT=1.
.PHONY: bench-compare
bench-compare: $(NATIVE_BENCH) $(NATIVE_SHORT_BENCH) $(ALLOC_BENCH_DEPS) | out
	@if [ ! -f $(BENCH_BASELINE)-throughput.csv ]; then echo "No baseline. Run: make bench-save first."; exit 1; fi
	@if [ ! -f $(BENCH_BASELINE)-latency.csv ]; then echo "No latency baseline. Run: make bench-save first."; exit 1; fi
	@rm -f out/bench-compare-failed
	@python3 "$(PERF_TOOL_DIR)/repeat_short_bench.py" --binary $(NATIVE_BENCH) --runs $(BENCH_RUNS) --cpu "$(BENCH_CPU)" --raw-output out/bench-current-throughput-runs.csv > out/bench-current-throughput.csv
	@python3 "$(PERF_TOOL_DIR)/repeat_short_bench.py" --binary $(NATIVE_SHORT_BENCH) --runs $(BENCH_RUNS) --cpu "$(BENCH_CPU)" --raw-output out/bench-current-latency-runs.csv > out/bench-current-latency.csv
	@echo ""
	@if [ -f $(BENCH_BASELINE)-info.txt ]; then echo "Baseline : $$(cat $(BENCH_BASELINE)-info.txt)"; else echo "Baseline : metadata unavailable"; fi
	@echo "Current  : commit=$$(git rev-parse --short HEAD 2>/dev/null||echo unknown)  date=$$(date '+%Y-%m-%d %H:%M')  runs=$(BENCH_RUNS)  cpu=$(BENCH_CPU)"
	@echo ""
	@echo "━━━  Throughput regression check  (median_ns per case×size; positive = slower)  ━━━"
	@awk -F, -v threshold=$(BENCH_REGRESSION_THRESHOLD) -v strict=$(BENCH_STRICT) '\
	  NR==FNR && FNR>1 { k=$$1 SUBSEP $$2; base[k]=$$3; hash[k]=$$6; baseCount++; next } \
	  FNR>1 { \
	    k=$$1 SUBSEP $$2; currentCount++; \
	    if (!(k in base)) { printf "%-28s %8s B  MISSING BASELINE\n",$$1,$$2; bad=1; next } \
	    pct=($$3-base[k])/base[k]*100; \
	    reg=(pct>=threshold); improved=(pct<=-threshold); \
	    if (reg) regression=1; \
	    tag=reg?"  REGRESSION ↑":improved?"  improved ↓":""; \
	    if ("x" $$6 != "x" hash[k]) { tag=tag "  OUTPUT MISMATCH"; bad=1 } \
	    printf "%-28s %8s B  base=%10.0f  now=%10.0f  %+6.1f%%%s\n",$$1,$$2,base[k],$$3,pct,tag \
	  } \
	  END { \
	    if (currentCount != baseCount) { printf "case-count mismatch: baseline=%d current=%d\n",baseCount,currentCount; bad=1 } \
	    if (bad || (strict && regression)) exit 1 \
	  }' \
	  $(BENCH_BASELINE)-throughput.csv out/bench-current-throughput.csv || touch out/bench-compare-failed
	@echo ""
	@echo "━━━  Short-call latency regression check  (p50 ns; positive = slower)  ━━━"
	@awk -F, -v threshold=$(BENCH_REGRESSION_THRESHOLD) -v strict=$(BENCH_STRICT) '\
	  NR==FNR && FNR>1 { k=$$1 SUBSEP $$2; base[k]=$$3; hash[k]=$$6; baseCount++; next } \
	  FNR>1 { \
	    k=$$1 SUBSEP $$2; currentCount++; \
	    if (!(k in base)) { printf "%-28s %8s B  MISSING BASELINE\n",$$1,$$2; bad=1; next } \
	    pct=($$3-base[k])/base[k]*100; \
	    reg=(pct>=threshold); improved=(pct<=-threshold); \
	    if (reg) regression=1; \
	    tag=reg?"  REGRESSION ↑":improved?"  improved ↓":""; \
	    if ("x" $$6 != "x" hash[k]) { tag=tag "  OUTPUT MISMATCH"; bad=1 } \
	    printf "%-28s %8s B  base=%7.0f ns  now=%7.0f ns  %+6.1f%%%s\n",$$1,$$2,base[k],$$3,pct,tag \
	  } \
	  END { \
	    if (currentCount != baseCount) { printf "case-count mismatch: baseline=%d current=%d\n",baseCount,currentCount; bad=1 } \
	    if (bad || (strict && regression)) exit 1 \
	  }' \
	  $(BENCH_BASELINE)-latency.csv out/bench-current-latency.csv || touch out/bench-compare-failed
	@echo ""
	@if [ "$(ALLOC_PROBE_SUPPORTED)" = "1" ] && [ -f $(BENCH_BASELINE)-allocs.csv ] && [ -s $(BENCH_BASELINE)-allocs.csv ] && [ -f $(LINUX_ALLOC_PROBE) ]; then \
	  echo "━━━  Allocation regression check  (malloc+calloc+realloc events, requested bytes, peak live bytes)  ━━━"; \
	  printf "" > out/bench-current-allocs.csv; \
	  for mode in devanagari latin virtual-indic expansion protected mixed-protected; do \
	    raw=$$(LD_PRELOAD=$(LINUX_ALLOC_PROBE) $(NATIVE_ALLOC_BENCH) $$mode 1048576 1 0 2>&1 1>/dev/null); \
	    echo "$$raw" | awk -v m=$$mode -F'[ =,]+' '{ printf "%s,%d,%d,%s,%s\n",m,$$2+$$5+$$8,$$3+$$6+$$9,$$15,$$13 }'; \
	  done > out/bench-current-allocs.csv; \
	  awk -F, -v threshold=$(BENCH_REGRESSION_THRESHOLD) -v strict=$(BENCH_STRICT) '\
	    NR==FNR { bt[$$1]=$$2; bb[$$1]=$$3; bp[$$1]=$$4; baseCount++; next } \
	    { \
	      currentCount++; \
	      if (!($$1 in bt)) { printf "%-22s  MISSING BASELINE\n",$$1; bad=1; next } \
	      tp=(bt[$$1]==0)?(($$2==0)?0:100):($$2-bt[$$1])/bt[$$1]*100; \
	      bpct=(bb[$$1]==0)?(($$3==0)?0:100):($$3-bb[$$1])/bb[$$1]*100; \
	      pp=(bp[$$1]==0)?(($$4==0)?0:100):($$4-bp[$$1])/bp[$$1]*100; \
	      tr=(tp>=threshold); br=(bpct>=threshold); pr=(pp>=threshold); \
	      if (tr || br || pr) regression=1; \
	      tt=tr?"  REGRESSION ↑":(tp<=-threshold)?"  improved ↓":""; \
	      btg=br?"  REGRESSION ↑":(bpct<=-threshold)?"  improved ↓":""; \
	      pt=pr?"  REGRESSION ↑":(pp<=-threshold)?"  improved ↓":""; \
	      printf "%-22s  allocs: %6d→%6d (%+.1f%%)%s   bytes: %10d→%10d (%+.1f%%)%s   peak: %10d→%10d (%+.1f%%)%s\n", \
	        $$1,bt[$$1],$$2,tp,tt,bb[$$1],$$3,bpct,btg,bp[$$1],$$4,pp,pt \
	    } \
	    END { \
	      if (currentCount != baseCount) { printf "case-count mismatch: baseline=%d current=%d\n",baseCount,currentCount; bad=1 } \
	      if (bad || (strict && regression)) exit 1 \
	    }' \
	    $(BENCH_BASELINE)-allocs.csv out/bench-current-allocs.csv || touch out/bench-compare-failed; \
	elif [ "$(ALLOC_PROBE_SUPPORTED)" != "1" ]; then \
	  echo "━━━  Allocation regression check skipped: requires Linux with glibc  ━━━"; \
	elif [ ! -s $(BENCH_BASELINE)-allocs.csv ]; then \
	  echo "━━━  Allocation regression check skipped: baseline has no allocation data  ━━━"; \
	fi
	@echo ""
	@if [ -f out/bench-compare-failed ]; then \
	  echo "Benchmark comparison failed (output mismatch, missing case, or strict regression)."; \
	  exit 1; \
	fi

# Linux/glibc-only allocator event profile; does not affect production builds.
.PHONY: bench-alloc-linux
ifeq ($(ALLOC_PROBE_SUPPORTED),1)
bench-alloc-linux: $(NATIVE_ALLOC_BENCH) $(LINUX_ALLOC_PROBE)
	LD_PRELOAD=$(LINUX_ALLOC_PROBE) $(NATIVE_ALLOC_BENCH) $(ALLOC_BENCH_MODE) $(ALLOC_BENCH_BYTES) $(ALLOC_BENCH_REPETITIONS) $(ALLOC_BENCH_WARMUPS)
else
bench-alloc-linux:
	@echo "bench-alloc-linux requires Linux with glibc."
	@exit 2
endif

out:
	mkdir -p out

$(NATIVE_BENCH): $(NATIVE_CPP) $(NATIVE_H) $(NATIVE_DIR)/bench/benchmark.cpp
	clang++ -std=c++23 -O3 -DNDEBUG -I $(NATIVE_SRC) $(NATIVE_CPP) $(NATIVE_DIR)/bench/benchmark.cpp -o $@

$(NATIVE_SHORT_BENCH): $(NATIVE_CPP) $(NATIVE_H) $(NATIVE_DIR)/bench/short_call.cpp
	clang++ -std=c++23 -O3 -DNDEBUG -I $(NATIVE_SRC) $(NATIVE_CPP) $(NATIVE_DIR)/bench/short_call.cpp -o $@

$(NATIVE_LOOKUP_BENCH): $(NATIVE_H) $(NATIVE_DIR)/bench/lookup.cpp | out
	clang++ -std=c++23 -O3 -DNDEBUG -I $(NATIVE_SRC) $(NATIVE_DIR)/bench/lookup.cpp -o $@

$(NATIVE_LOOKUP_ALLOC_BENCH): $(NATIVE_H) $(NATIVE_DIR)/bench/lookup.cpp | out
	clang++ -std=c++23 -O3 -DNDEBUG -DINDTRANSLIT_ALLOC_PROBE -I $(NATIVE_SRC) $(NATIVE_DIR)/bench/lookup.cpp -ldl -o $@

$(NATIVE_COLD_BENCH): $(NATIVE_CPP) $(NATIVE_H) $(NATIVE_DIR)/bench/cold_start.cpp | out
	clang++ -std=c++23 -O3 -DNDEBUG -I $(NATIVE_SRC) $(NATIVE_DIR)/bench/cold_start.cpp $(NATIVE_CPP) -o $@

$(NATIVE_MEM_BENCH): $(NATIVE_CPP) $(NATIVE_H) $(NATIVE_DIR)/bench/memory.cpp
	clang++ -std=c++23 -O3 -DNDEBUG -I $(NATIVE_SRC) $(NATIVE_CPP) $(NATIVE_DIR)/bench/memory.cpp -o $@

ifeq ($(ALLOC_PROBE_SUPPORTED),1)
$(NATIVE_ALLOC_BENCH): $(NATIVE_CPP) $(NATIVE_H) $(NATIVE_DIR)/bench/memory.cpp
	clang++ -std=c++23 -O3 -DNDEBUG -DINDTRANSLIT_ALLOC_PROBE -I $(NATIVE_SRC) $(NATIVE_CPP) $(NATIVE_DIR)/bench/memory.cpp -ldl -o $@

$(LINUX_ALLOC_PROBE): $(NATIVE_DIR)/bench/allocation_probe_linux.c
	clang -std=c11 -O2 -fPIC -shared $(NATIVE_DIR)/bench/allocation_probe_linux.c -o $@
endif

test_flutter: wasm flutter/lib/src/bindings.dart
	cd flutter/example && flutter run -d chrome

test_nodejs:
	cd nodejs && yarn && yarn test

cli: $(NATIVE_CLI)
	$(NATIVE_CLI)
