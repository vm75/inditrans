#!/usr/bin/env bash
set -euo pipefail

CDPATH= cd "$(dirname "${BASH_SOURCE[0]}")/../.."
build_dir="${SECURITY_BUILD_DIR:-out/security}"
cxx="${CXX:-clang++}"
mkdir -p "$build_dir"
flags=(-std=c++23 -O1 -g -fno-omit-frame-pointer -fsanitize=fuzzer,address,undefined,enum -I native/src)

"$cxx" "${flags[@]}" native/src/inditrans.cpp native/security/fuzz_transliterate.cpp -o "$build_dir/fuzz_transliterate"
"$cxx" "${flags[@]}" native/security/fuzz_utf8.cpp -o "$build_dir/fuzz_utf8"
"$cxx" "${flags[@]}" native/security/fuzz_marker_filter.cpp -o "$build_dir/fuzz_marker_filter"
"$cxx" "${flags[@]}" native/src/inditrans.cpp native/security/fuzz_preservation.cpp -o "$build_dir/fuzz_preservation"
"$cxx" "${flags[@]}" native/src/inditrans.cpp native/security/fuzz_c_abi.cpp -o "$build_dir/fuzz_c_abi"
"$cxx" "${flags[@]}" native/security/fuzz_tamil_marker.cpp -o "$build_dir/fuzz_tamil_marker"
python3 native/security/seed_corpus.py --output "$build_dir/corpus"
printf 'Fuzzers and fixture-derived corpora are ready under %s\n' "$build_dir"
