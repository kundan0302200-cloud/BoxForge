#!/usr/bin/env bash
set -euo pipefail
ROOTFS="${1:-./rootfs/busybox}"
[[ $EUID -eq 0 ]] || { echo "Run with sudo"; exit 1; }
[[ -x ./boxforge ]] || { echo "Build first: make"; exit 1; }

echo "== PID namespace =="
./boxforge run "$ROOTFS" /bin/sh -c 'echo "PID=$$"; ps'
echo "== UTS namespace =="
./boxforge run --hostname boxforge-test "$ROOTFS" /bin/hostname
echo "Isolation smoke test complete."
