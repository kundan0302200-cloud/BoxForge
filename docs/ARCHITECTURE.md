# BoxForge Architecture

```text
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
     |
     +--> Filesystem Manager
     |      +-- rootfs
     |      +-- pivot_root
     |      +-- /proc
     |      +-- OverlayFS (advanced)
     |
     +--> Resource Manager
     |      +-- cgroup v2
     |      +-- memory.max
     |      +-- cpu.max
     |
     +--> Lifecycle Manager
            +-- PID 1
            +-- signals
            +-- wait/reaping
            +-- cleanup
```

## Security note

This is an educational runtime. A production runtime needs substantially more security hardening, capability management, seccomp, LSM integration, robust device policies, race-resistant path handling, and OCI compatibility.
