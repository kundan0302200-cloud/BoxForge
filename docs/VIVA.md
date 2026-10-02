# Viva Questions

## What is BoxForge?
A minimal educational Linux container runtime written in C11.

## Why namespaces?
They isolate the visibility of processes, mounts, hostname, IPC and networking.

## Why cgroups?
They control resource usage such as CPU and memory.

## Why PID 1?
The first process in a PID namespace is PID 1 and has special child-reaping responsibilities.

## Why BusyBox?
It provides a compact set of Unix utilities suitable for a small root filesystem.

## What is pivot_root?
It changes the process root to a new filesystem and allows the old root to be detached.

## Is this Docker?
No. It is a smaller educational implementation of core Linux container mechanisms.
