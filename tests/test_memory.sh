#!/usr/bin/env bash
set -euo pipefail
ROOTFS="${1:-./rootfs/busybox}"
[[ $EUID -eq 0 ]] || { echo "Run with sudo"; exit 1; }

echo "Starting a 64M-limited container. A real stress test requires a workload that allocates memory."
./boxforge run --memory 64M "$ROOTFS" /bin/sh -c 'echo "memory.max test container PID=$$"; cat /sys/fs/cgroup/memory.max 2>/dev/null || true; sleep 1'
