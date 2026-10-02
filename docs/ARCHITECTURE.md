# BoxForge Architecture

~~~text
boxforge CLI
     |
     v
Container Manager
     |
     +--> Namespace Manager
     |      +-- PID
     |      +-- Mount
     |      +-- UTS
     |      +-- IPC
     |      +-- Network
     |      +-- User (optional)
     |
     +--> Filesystem Manager
     |      +-- pivot_root
     |      +-- /proc
     |      +-- /sys
     |      +-- minimal devices
     |
     +--> Resource Manager
     |      +-- cgroup v2
     |      +-- memory.max
     |      +-- cpu.max
     |      +-- startup barrier
     |
     +--> Lifecycle Manager
            +-- PID 1 payload
            +-- signal forwarding
            +-- wait/exit status
            +-- cgroup cleanup
~~~

## Startup sequence

1. Create a synchronization pipe.
2. clone the container child with the requested namespaces.
3. Keep the child blocked before workload execution.
4. If requested, configure UID/GID maps from the parent.
5. Create the cgroup and apply CPU/memory limits.
6. Release the child.
7. Configure hostname and private mount propagation.
8. pivot_root into the supplied rootfs.
9. Mount /proc and attempt /sys.
10. Expose only a small device set.
11. Set a container-oriented PATH/HOME and execute the command.
12. Forward parent SIGINT/SIGTERM and wait for the container process.
13. Remove the cgroup after exit.

## Security note

BoxForge remains an educational runtime. It is not intended for untrusted workloads. Production isolation requires additional capabilities, seccomp, LSM policy, device policy, robust path handling and image verification.
