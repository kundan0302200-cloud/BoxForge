#ifndef BOXFORGE_CGROUP_H
#define BOXFORGE_CGROUP_H
#include <sys/types.h>
int cgroup_create(const char *id, const char *memory, const char *cpu, pid_t pid);
int cgroup_remove(const char *id);
#endif
