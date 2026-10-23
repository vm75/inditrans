#!/usr/bin/env bash
set -euo pipefail

CDPATH= cd "$(dirname "${BASH_SOURCE[0]}")/../.."
base_ref="${AUDIT_BASE_REF:-fdaf6b5}"
trace_dir="${AUDIT_TRACE_DIR:-out/security/baseline-traces}"
audit_tmp="$(mktemp -d "${TMPDIR:-/tmp}/inditrans-audit-XXXXXX")"
trap 'rm -rf "$audit_tmp"' EXIT
mkdir -p "$trace_dir"

git archive "$base_ref" native/src | tar -x -C "$audit_tmp"
cxx="${CXX:-clang++}"
"$cxx" -std=c++23 -O1 -g -fno-omit-frame-pointer -fno-sanitize-recover=all \
  -fsanitize=address,undefined,enum -I "$audit_tmp/native/src" \
  "$audit_tmp/native/src/inditrans.cpp" native/security/baseline_reproducer.cpp \
  -Wl,--wrap=realloc -o "$audit_tmp/baseline_reproducer"

for mode in truncated-marker output-alias invalid-options null-text null-from null-to \
  null-skip-start null-skip-end null-script-query builder-replace-short \
  builder-realloc-failure default-empty-view; do
  ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 \
    "$audit_tmp/baseline_reproducer" "$mode" > "$trace_dir/$mode.log" 2>&1 || true
done

ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 LSAN_OPTIONS=exitcode=23 \
  "$audit_tmp/baseline_reproducer" flutter-missing-release \
  > "$trace_dir/flutter-missing-release.log" 2>&1 || true

grep -hE 'ERROR: AddressSanitizer|runtime error:|SUMMARY: (AddressSanitizer|UndefinedBehaviorSanitizer)|LeakSanitizer:' \
  "$trace_dir"/*.log || true
