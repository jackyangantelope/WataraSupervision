#!/usr/bin/env bash
set -euo pipefail
if [ "$#" -ne 1 ]; then
    echo 'Usage: bash scripts/test-host.sh /path/to/user-game.sv' >&2
    exit 2
fi
game=$(realpath "$1")
mkdir -p build/host
make -f tests/host.mk -j4
for mode in idle keyboard pad; do
    ASAN_OPTIONS=detect_leaks=1 build/host/sv "$game" - "build/host/$mode" "$mode" 3600
done
