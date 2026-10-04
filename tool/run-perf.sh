#!/bin/bash

set -e

export CPU=0
export RUNS=5

run_current=false
case "${1:-}" in
  --current) run_current=true ;;
  "") ;;
  *) echo "Usage: $0 [--current]" >&2; exit 2 ;;
esac
if (( $# > 1 )); then
  echo "Usage: $0 [--current]" >&2
  exit 2
fi

repo_root=$(git rev-parse --show-toplevel)
cd "$repo_root"

current_commit=$(git rev-parse HEAD)
current_branch=$(git symbolic-ref --quiet --short HEAD || true)
stash_commit=""
checkout_started=false

restore_workspace() {
  local status=$? stash_hash stash_ref
  trap - EXIT

  if "$checkout_started"; then
    if ! git restore flutter/assets/inditrans.wasm; then
      echo "Could not restore the generated Wasm file; pending changes remain stashed." >&2
      exit 1
    fi
    if [[ -n "$current_branch" ]]; then
      if ! git switch "$current_branch"; then
        echo "Could not restore branch $current_branch; pending changes remain stashed." >&2
        exit 1
      fi
    elif ! git checkout --detach "$current_commit"; then
      echo "Could not restore commit $current_commit; pending changes remain stashed." >&2
      exit 1
    fi
  fi

  if [[ -n "$stash_commit" ]]; then
    if ! git stash apply --index "$stash_commit"; then
      echo "Could not fully restore pending changes; stash $stash_commit was retained." >&2
      exit 1
    fi
    while read -r stash_hash stash_ref; do
      if [[ "$stash_hash" == "$stash_commit" ]]; then
        git stash drop "$stash_ref" || exit 1
        break
      fi
    done < <(git stash list --format='%H %gd')
  fi
  exit "$status"
}
trap restore_workspace EXIT
trap 'exit 130' INT
trap 'exit 143' TERM

if [[ -n "$(git status --porcelain --untracked-files=all)" ]]; then
  git stash push --include-untracked -m "Before performance benchmarks"
  stash_commit=$(git rev-parse refs/stash)
fi
checkout_started=true

run_snapshot() {
  local ref="$1" name="$2"
  echo "==> Running snapshot for $name ($ref)..."

  git restore flutter/assets/inditrans.wasm
  git checkout --detach "$ref"

  make perf-snapshot \
    PERF_NAME="$name" \
    BENCH_RUNS="$RUNS" \
    BENCH_CPU="$CPU" \
    PERF_CPU="$CPU"
}

mapfile -t TAGS < <(git tag --list 'perf-*' | LC_ALL=C sort)

for ref in "${TAGS[@]}"; do
  name="${ref#perf-}"
  if [[ -f "./out/perf/$name-wasm-size.txt" ]] ; then
    echo "Skipping $name"
    echo
    continue
  fi

  run_snapshot "$ref" "$name"
done

if "$run_current"; then
  run_snapshot "$current_commit" 99-current
fi

make perf-report PERF_BASELINE=00-baseline > out/report.md
