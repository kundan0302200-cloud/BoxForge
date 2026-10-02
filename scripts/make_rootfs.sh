#!/usr/bin/env bash
set -euo pipefail
ROOTFS="${1:-./rootfs/busybox}"
BUSYBOX="${BUSYBOX:-$(command -v busybox || true)}"
if [[ -z "$BUSYBOX" ]]; then
  echo "busybox not found. Install it with: sudo apt install busybox-static" >&2
  exit 1
fi
mkdir -p "$ROOTFS"/{bin,dev,etc,proc,sys,tmp}
cp -f "$BUSYBOX" "$ROOTFS/bin/busybox"
for app in sh ls cat echo ps pwd mkdir rm mount umount sleep uname hostname; do
  ln -sf busybox "$ROOTFS/bin/$app"
done
chmod 1777 "$ROOTFS/tmp"
echo "Created BusyBox rootfs at $ROOTFS"
