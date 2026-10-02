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

static int child_main(void *arg) {
    ContainerConfig *cfg = arg;
    char release;

    if (cfg->sync_fd < 0 || read(cfg->sync_fd, &release, 1) != 1)
        _exit(124);
    close(cfg->sync_fd);

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
    if (!cfg || !cfg->argv || !cfg->argv[0]) {
        errno = EINVAL;
        return -1;
    }

    int sync_pipe[2];
    if (pipe(sync_pipe) < 0)
        return -1;

    static char stack[BF_STACK_SIZE] __attribute__((aligned(16)));
    cfg->sync_fd = sync_pipe[0];

    pid_t pid = clone(child_main, stack + sizeof(stack),
                      namespace_clone_flags(cfg), cfg);
    if (pid < 0) {
        close(sync_pipe[0]);
        close(sync_pipe[1]);
        return -1;
    }

    cfg->child_pid = pid;
    close(sync_pipe[0]);

    if (cfg->use_userns &&
        namespace_configure_userns(pid, cfg->host_uid, cfg->host_gid) < 0) {
        int saved = errno;
        bf_log("user namespace mapping failed: %s", strerror(saved));
        kill(pid, SIGKILL);
        close(sync_pipe[1]);
        waitpid(pid, NULL, 0);
        errno = saved;
        return -1;
    }

    if (cgroup_create(cfg->id, cfg->memory_limit, cfg->cpu_limit, pid) < 0) {
        int saved = errno;
        bf_log("cgroup setup failed: %s", strerror(saved));
        kill(pid, SIGKILL);
        close(sync_pipe[1]);
        waitpid(pid, NULL, 0);
        cgroup_remove(cfg->id);
        errno = saved;
        return -1;
    }

    if (write(sync_pipe[1], "1", 1) != 1) {
        int saved = errno ? errno : EIO;
        kill(pid, SIGKILL);
        close(sync_pipe[1]);
        waitpid(pid, NULL, 0);
        cgroup_remove(cfg->id);
        errno = saved;
        return -1;
    }
    close(sync_pipe[1]);

    int status = 0;
    if (lifecycle_wait(pid, &status) < 0) {
        int saved = errno;
        kill(pid, SIGKILL);
        waitpid(pid, NULL, 0);
        cgroup_remove(cfg->id);
        errno = saved;
        return -1;
    }

    if (cgroup_remove(cfg->id) < 0)
        bf_log("warning: failed to remove cgroup %s: %s",
               cfg->id, strerror(errno));

    if (WIFEXITED(status)) return WEXITSTATUS(status);
    if (WIFSIGNALED(status)) return 128 + WTERMSIG(status);
    return 1;
}
