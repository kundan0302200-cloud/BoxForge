# Implementation Roadmap

## Completed core

1. CLI + exec foundation
2. PID + UTS namespaces
3. Mount + IPC + network namespaces
4. BusyBox rootfs + /proc
5. PID 1 payload lifecycle and parent signal forwarding
6. cgroups v2 CPU and memory limits
7. isolation, memory and teardown tests
8. optional user namespace UID/GID mapping

## Next

9. boxforge exec using setns
10. OverlayFS lower/upper rootfs
11. veth + bridge + NAT networking
12. tar image/unpack format
13. persistent namespace handles
14. dedicated init supervisor and expanded integration tests
