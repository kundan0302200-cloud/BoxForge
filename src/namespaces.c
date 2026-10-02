#include "boxforge.h"
#include <errno.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/mount.h>
#include <unistd.h>

int namespace_clone_flags(const ContainerConfig *cfg) {
    int flags = CLONE_NEWPID | CLONE_NEWNS | CLONE_NEWUTS |
                CLONE_NEWIPC | CLONE_NEWNET | SIGCHLD;
    if (cfg && cfg->use_userns)
        flags |= CLONE_NEWUSER;
    return flags;
}

static int write_proc_file(pid_t pid, const char *name, const char *value) {
    char path[128];
    int n = snprintf(path, sizeof(path), "/proc/%d/%s", (int)pid, name);
    if (n < 0 || (size_t)n >= sizeof(path)) { errno = ENAMETOOLONG; return -1; }

    FILE *fp = fopen(path, "w");
    if (!fp) return -1;
    if (fputs(value, fp) < 0) {
        int saved = errno;
        fclose(fp);
        errno = saved;
        return -1;
    }
    if (fclose(fp) != 0) return -1;
    return 0;
}

int namespace_configure_userns(pid_t pid, uid_t host_uid, gid_t host_gid) {
    char map[128];

    if (write_proc_file(pid, "setgroups", "deny\n") < 0 && errno != ENOENT)
        return -1;

    int n = snprintf(map, sizeof(map), "0 %u 1\n", (unsigned)host_uid);
    if (n < 0 || (size_t)n >= sizeof(map)) { errno = ENAMETOOLONG; return -1; }
    if (write_proc_file(pid, "uid_map", map) < 0)
        return -1;

    n = snprintf(map, sizeof(map), "0 %u 1\n", (unsigned)host_gid);
    if (n < 0 || (size_t)n >= sizeof(map)) { errno = ENAMETOOLONG; return -1; }
    if (write_proc_file(pid, "gid_map", map) < 0)
        return -1;

    return 0;
}

int namespace_setup(ContainerConfig *cfg) {
    if (sethostname(cfg->hostname ? cfg->hostname : "boxforge",
                    cfg->hostname ? strlen(cfg->hostname) : 8) < 0)
        return -1;

    return mount(NULL, "/", NULL, MS_REC | MS_PRIVATE, NULL);
}
