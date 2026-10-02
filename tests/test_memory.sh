#!/usr/bin/env bash
set -euo pipefail
ROOTFS="${1:-./rootfs/busybox}"
[[ $EUID -eq 0 ]] || { echo "Run with sudo"; exit 1; }

MEMHOG=/tmp/boxforge-memhog
trap 'rm -f "$MEMHOG" "$ROOTFS/bin/memhog"' EXIT

if ! cc -static -O2 tests/memhog.c -o "$MEMHOG" 2>/dev/null; then
    echo "ERROR: static compiler support is required for the memory test."
    exit 1
fi

cp "$MEMHOG" "$ROOTFS/bin/memhog"
set +e
./boxforge run --memory 64M "$ROOTFS" /bin/memhog
rc=$?
set -e

if [[ $rc -ne 137 ]]; then
    echo "ERROR: expected memory.max to OOM-kill memhog (exit 137), got $rc"
    exit 1
fi
echo "Memory enforcement test passed."
