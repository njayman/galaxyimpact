#!/usr/bin/env bash
# Build with ASan + UBSan and run the --autoplay bot under Xvfb.
# Fails on any sanitizer report. Audio is muted via a scratch XDG_DATA_HOME.
# Usage: tools/asan_autoplay.sh [build-dir]   (default: build-asan)
set -euo pipefail

root="$(cd "$(dirname "$0")/.." && pwd)"
build="${1:-$root/build-asan}"
run_seconds="${RUN_SECONDS:-240}"  # bot runs 120 s; headroom for loading

cmake -S "$root" -B "$build" -G Ninja -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer -O1 -g" \
    -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined" >/dev/null
cmake --build "$build" -j"$(nproc)"

scratch="$(mktemp -d)"
trap 'rm -rf "$scratch"' EXIT
mkdir -p "$scratch/xdg"

set +e
cd "$build"
XDG_DATA_HOME="$scratch/xdg" \
ASAN_OPTIONS="detect_leaks=1" \
UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1" \
timeout "$run_seconds" xvfb-run -a -s "-screen 0 1280x720x24" ./GalaxyImpact --autoplay >"$scratch/run.log" 2>&1
status=$?
set -e

if grep -q "ERROR: AddressSanitizer\|runtime error\|ERROR: LeakSanitizer" "$scratch/run.log" || [ "$status" -ne 0 ]; then
    cat "$scratch/run.log"
    echo "asan_autoplay: FAILED (exit $status)" >&2
    exit 1
fi
echo "asan_autoplay: clean (exit $status)"
