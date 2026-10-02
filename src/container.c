#define _GNU_SOURCE
#include "boxforge.h"
#include "namespaces.h"
#include "cgroup.h"
#include "filesystem.h"
#include "lifecycle.h"
#include <errno.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static ContainerConfig *active_cfg;

static int child_main(void *arg) {
    ContainerConfig *cfg = arg;

    if (namespace_setup(cfg) < 0) {
        perror("namespace setup");
        _exit(125);
    }

    if (filesystem_setup(cfg->rootfs) < 0) {
        perror("filesystem setup");
        _exit(126);
    }

    if (lifecycle_init() < 0) {
        perror("lifecycle init");
        _exit(127);
    }

    execvp(cfg->argv[0], cfg->argv);
    perror("execvp");
    _exit(127);
}

int container_run(ContainerConfig *cfg) {
    if (!cfg || !cfg->argv || !cfg->argv[0]) { errno = EINVAL; return -1; }

    static char stack[BF_STACK_SIZE] __attribute__((aligned(16)));
    active_cfg = cfg;
    int flags = namespace_clone_flags();
    pid_t pid = clone(child_main, stack + sizeof(stack), flags, cfg);
    if (pid < 0) return -1;
    cfg->child_pid = pid;

    /* The parent creates the cgroup after clone, then moves the child into it. */
    if (cgroup_create(cfg->id, cfg->memory_limit, cfg->cpu_limit, pid) < 0) {
        bf_log("cgroup setup failed: %s", strerror(errno));
        kill(pid, SIGKILL);
        waitpid(pid, NULL, 0);
        return -1;
    }

    int status = 0;
    if (lifecycle_wait(pid, &status) < 0) {
        cgroup_remove(cfg->id);
        return -1;
    }

    if (cgroup_remove(cfg->id) < 0)
        bf_log("warning: failed to remove cgroup %s: %s", cfg->id, strerror(errno));

    if (WIFEXITED(status)) return WEXITSTATUS(status);
    if (WIFSIGNALED(status)) return 128 + WTERMSIG(status);
    return 1;
}
