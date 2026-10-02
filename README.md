# BoxForge

BoxForge is a minimal educational Linux container runtime written in C11.

## Implemented

- boxforge run <rootfs> <command> [args...]
- PID, mount, UTS, IPC and network namespaces
- optional user namespace with UID/GID mapping
- private mount propagation
- pivot_root filesystem isolation
- /proc and best-effort /sys
- minimal device exposure instead of the host /dev tree
- cgroups v2 memory.max and cpu.max
- startup synchronization so the workload is released only after cgroup setup
- parent SIGINT/SIGTERM forwarding
- BusyBox rootfs helper
- interactive CLI

## Advanced / planned

- boxforge exec using setns
- OverlayFS lower/upper rootfs
- veth + bridge + NAT
- tar image/unpack
- persistent namespace handles
- dedicated PID 1 supervisor for arbitrary workloads

## Build

~~~bash
make
~~~

## Prepare BusyBox

~~~bash
sudo ./scripts/make_rootfs.sh ./rootfs/busybox
~~~

## Run

~~~bash
sudo ./boxforge run ./rootfs/busybox /bin/sh
~~~

Inside:

~~~sh
echo $$
hostname
ps
cat /proc/self/cgroup
ls /
~~~

## Resource limits

~~~bash
sudo ./boxforge run --memory 100M --cpu 50% ./rootfs/busybox /bin/sh
~~~

Host-side cgroup inspection:

~~~bash
sudo find /sys/fs/cgroup -maxdepth 1 -type d -name 'boxforge-*'
sudo cat /sys/fs/cgroup/boxforge-*/memory.max
sudo cat /sys/fs/cgroup/boxforge-*/cpu.max
~~~

## User namespace

~~~bash
sudo ./boxforge run --userns ./rootfs/busybox /bin/sh
~~~

Container UID 0 is mapped to the launching host UID/GID. Availability depends on the host kernel and namespace policy.

## Tests

~~~bash
sudo ./tests/test_isolation.sh
sudo ./tests/test_memory.sh
sudo ./tests/test_cleanup.sh
~~~

The memory test builds a small static allocator workload and expects a 64 MiB cgroup to terminate it.

## Security note

BoxForge is an educational runtime, not a production security boundary. Production runtimes require additional capability, seccomp, LSM, device-policy, path-handling and image-management hardening.
