#!/usr/bin/env bash
# Run the --autoplay bot over many RNG seeds on an ASan/UBSan build and report failures.
# A run fails on a sanitizer report, a broken invariant (see autoplayInvariantFailure in
# src/sandbox.cpp) or a non-zero exit. Failing logs are kept in the output directory.
# Usage: tools/autoplay_seeds.sh [build-dir] [first-seed] [count] [seconds]
# Env: PARALLEL (default 4), OUT (default a fresh dir under the scratch area).
set -uo pipefail

root="$(cd "$(dirname "$0")/.." && pwd)"
build="${1:-$root/build-asan}"
first="${2:-1}"
count="${3:-8}"
seconds="${4:-60}"
parallel="${PARALLEL:-4}"
out="${OUT:-$(mktemp -d)}"
mkdir -p "$out"

cmake --build "$build" -j"$(nproc)" >/dev/null || { echo "build failed" >&2; exit 1; }

run_seed() {
    local seed="$1" log="$2"
    local scratch
    scratch="$(mktemp -d)"
    (
        cd "$build" || exit 1
        XDG_DATA_HOME="$scratch" \
        ASAN_OPTIONS="detect_leaks=0" \
        UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1" \
        timeout "$((seconds + 120))" xvfb-run -a -s "-screen 0 1280x720x24" \
            ./GalaxyImpact --autoplay --seed "$seed" --autoplay-seconds "$seconds" >"$log" 2>&1
    )
    local status=$?
    rm -rf "$scratch"
    if [ "$status" -ne 0 ] || grep -q "ERROR: AddressSanitizer\|runtime error" "$log"; then
        echo "FAIL seed=$seed exit=$status log=$log"
        return 1
    fi
    echo "ok seed=$seed"
}
export -f run_seed
export build seconds

seq "$first" "$((first + count - 1))" |
    xargs -P "$parallel" -I{} bash -c 'run_seed "$1" "$2/seed-$1.log"' _ {} "$out"
echo "logs: $out"
