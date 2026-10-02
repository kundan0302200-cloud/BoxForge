# BoxForge

A minimal educational Linux container runtime written in C11.

## Current implementation

Core runtime features:
- `boxforge run <rootfs> <command> [args...]`
- PID, mount, UTS, IPC and network namespaces
- private mount propagation
- root filesystem isolation with `pivot_root`
- `/proc` mount inside the container
- cgroups v2 CPU and memory limits
- PID 1 child reaping
- signal-aware lifecycle and cleanup
- BusyBox rootfs helper

Planned/advanced:
- `boxforge exec` using `setns`
- OverlayFS lower/upper rootfs
- user namespace UID/GID mapping
- veth + bridge + NAT networking
- tar image format/unpack

## Requirements

Linux, GCC, GNU Make, and root/CAP_SYS_ADMIN for the privileged features.

Recommended: a native Ubuntu VM or Linux machine. WSL2 can work for many features, but kernel/cgroup/network support depends on configuration.

## Build

```bash
make
```

## Prepare a BusyBox rootfs

```bash
sudo ./scripts/make_rootfs.sh ./rootfs/busybox
```

## Run

```bash
sudo ./boxforge run ./rootfs/busybox /bin/sh
```

Inside:

```sh
hostname
ps
cat /proc/1/status | head
ls /
```

## Resource limits

```bash
sudo ./boxforge run --memory 100M --cpu 50% ./rootfs/busybox /bin/sh
```

## Tests

```bash
sudo ./tests/test_isolation.sh
sudo ./tests/test_memory.sh
sudo ./tests/test_cleanup.sh
```

> This is an educational runtime, not a production security boundary. Do not run untrusted workloads with it.
