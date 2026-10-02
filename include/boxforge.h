#ifndef BOXFORGE_H
#define BOXFORGE_H

#define _GNU_SOURCE
#include <limits.h>
#include <sys/types.h>

#define BF_MAX_ID 64
#define BF_STACK_SIZE (1024 * 1024)

typedef struct {
    const char *rootfs;
    char **argv;
    const char *hostname;
    const char *memory_limit;
    const char *cpu_limit;
    int use_userns;
    int verbose;
    pid_t child_pid;
    char id[BF_MAX_ID];
} ContainerConfig;

int container_run(ContainerConfig *cfg);
int namespace_setup(ContainerConfig *cfg);
int filesystem_setup(const char *rootfs);
int cgroup_create(const char *id, const char *memory, const char *cpu, pid_t pid);
int cgroup_remove(const char *id);
int lifecycle_init(void);
int lifecycle_wait(pid_t pid, int *status);
void bf_die(const char *msg);
void bf_log(const char *fmt, ...);
int parse_size_bytes(const char *s, unsigned long long *out);
int parse_cpu_percent(const char *s, unsigned long long *quota, unsigned long long *period);

#endif
