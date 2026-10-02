# Viva Questions

## What is BoxForge?
A minimal educational Linux container runtime written in C11.

## Why namespaces?
They provide separate kernel views. BoxForge uses PID, mount, UTS, IPC and network namespaces, with an optional user namespace.

## Why cgroups?
cgroups v2 control resource usage. BoxForge uses memory.max and cpu.max.

## Why is the cgroup startup barrier important?
If the cgroup is created after the child starts, the workload can briefly run without its intended limits. BoxForge blocks the child until the parent finishes cgroup setup.

## Why PID 1?
The first process in a PID namespace is PID 1 and has special lifecycle semantics. The basic BoxForge run path makes the requested payload PID 1. A dedicated supervisor for arbitrary workloads remains an advanced hardening item.

## What is pivot_root?
It changes the process root to a new filesystem tree and allows the old root to be detached.

## Why private mount propagation?
It prevents mount changes from propagating between the container and host mount trees.

## Why not bind the whole host /dev?
That would expose many host devices. BoxForge exposes only a small set needed by basic Unix programs.

## What does --userns do?
It creates a user namespace and maps container UID/GID 0 to the launching host UID/GID.

## Why BusyBox?
It provides many Unix utilities in a compact binary, making a small rootfs practical.

## How are CPU and memory limits represented?
For example, 50% CPU uses cpu.max quota 50000 with a 100000 microsecond period. 100M memory is written as the corresponding byte value to memory.max.

## How does cleanup work?
The parent waits for the container and removes its boxforge-* cgroup. The cleanup test repeats ten runs and fails if any cgroup remains.

## Is this Docker?
No. BoxForge is a small educational implementation of Linux container mechanisms, not a production Docker-compatible runtime.
