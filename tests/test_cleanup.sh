#!/usr/bin/env bash
set -euo pipefail
ROOTFS="${1:-./rootfs/busybox}"
[[ $EUID -eq 0 ]] || { echo "Run with sudo"; exit 1; }

for i in $(seq 1 10); do
    echo "cycle $i"
    ./boxforge run "$ROOTFS" /bin/true
done

leftovers=$(find /sys/fs/cgroup -maxdepth 1 -type d -name 'boxforge-*' -print)
if [[ -n "$leftovers" ]]; then
    echo "ERROR: BoxForge cgroups remain:"
    echo "$leftovers"
    exit 1
fi
echo "Cleanup test passed: zero BoxForge cgroups remain."
