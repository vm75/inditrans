#!/usr/bin/env bash
set -euo pipefail

CDPATH= cd "$(dirname "${BASH_SOURCE[0]}")/../.."
build_dir="${SECURITY_BUILD_DIR:-out/security}"
cxx="${CXX:-clang++}"
flags=(
  -std=c++23 -O1 -g -fno-omit-frame-pointer -fno-sanitize-recover=all
  -fsanitize=address,undefined,enum -I native/src
)
mkdir -p "$build_dir"

"$cxx" "${flags[@]}" native/src/inditrans.cpp native/security/security_smoke.cpp -pthread -o "$build_dir/security_smoke"
ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 \
  "$build_dir/security_smoke"

"$cxx" "${flags[@]}" native/src/inditrans.cpp native/security/reproduce_findings.cpp \
  -Wl,--wrap=realloc -o "$build_dir/reproduce_findings"
python3 native/security/check_reproducers.py "$build_dir/reproduce_findings"
