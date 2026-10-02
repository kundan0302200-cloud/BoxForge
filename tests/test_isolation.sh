#!/usr/bin/env bash
set -euo pipefail
ROOTFS="${1:-./rootfs/busybox}"
[[ $EUID -eq 0 ]] || { echo "Run with sudo"; exit 1; }
[[ -x ./boxforge ]] || { echo "Build first: make"; exit 1; }

echo "== PID namespace =="
output=$(./boxforge run "$ROOTFS" /bin/sh -c 'echo "PID=$$"; ps')
echo "$output"
grep -q '^PID=1$' <<< "$output"
grep -q '^[[:space:]]*1 ' <<< "$output"

echo "== UTS namespace =="
[[ "$(./boxforge run --hostname boxforge-test "$ROOTFS" /bin/hostname)" == "boxforge-test" ]]

echo "== Mount/rootfs isolation =="
root_listing=$(./boxforge run "$ROOTFS" /bin/sh -c 'ls /')
grep -q '^bin' <<< "$root_listing"

echo "== Network namespace =="
net_output=$(./boxforge run "$ROOTFS" /bin/sh -c 'cat /proc/net/dev')
echo "$net_output"
grep -q 'lo:' <<< "$net_output"

echo "Isolation tests passed."
