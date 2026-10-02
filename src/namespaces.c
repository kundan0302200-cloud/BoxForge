#include "boxforge.h"
#include <errno.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/mount.h>
#include <unistd.h>

int namespace_clone_flags(void) {
    return CLONE_NEWPID | CLONE_NEWNS | CLONE_NEWUTS | CLONE_NEWIPC |
           CLONE_NEWNET | SIGCHLD;
}

static int setup_user_namespace(ContainerConfig *cfg) {
    char path[128], map[128];
    FILE *fp;

    if (snprintf(path, sizeof(path), "/proc/%d/setgroups", (int)getpid()) >= (int)sizeof(path))
        return -1;
    fp = fopen(path, "w");
    if (fp) {
        fputs("deny\n", fp);
        fclose(fp);
    }

    if (snprintf(path, sizeof(path), "/proc/%d/uid_map", (int)getpid()) >= (int)sizeof(path))
        return -1;
    if (snprintf(map, sizeof(map), "0 %u 1\n", (unsigned)cfg->host_uid) >= (int)sizeof(map))
        return -1;
    fp = fopen(path, "w");
    if (!fp) return -1;
    fputs(map, fp);
    fclose(fp);

    if (snprintf(path, sizeof(path), "/proc/%d/gid_map", (int)getpid()) >= (int)sizeof(path))
        return -1;
    if (snprintf(map, sizeof(map), "0 %u 1\n", (unsigned)cfg->host_gid) >= (int)sizeof(map))
        return -1;
    fp = fopen(path, "w");
    if (!fp) return -1;
    fputs(map, fp);
    fclose(fp);
    return 0;
}

int namespace_clone_flags(void) {
    return CLONE_NEWPID | CLONE_NEWNS | CLONE_NEWUTS | CLONE_NEWIPC |
           CLONE_NEWNET | SIGCHLD;
}

int namespace_setup(ContainerConfig *cfg) {
    if (cfg->use_userns && setup_user_namespace(cfg) < 0)
        return -1;

    if (sethostname(cfg->hostname ? cfg->hostname : "boxforge",
                    cfg->hostname ? strlen(cfg->hostname) : 8) < 0)
        return -1;

    if (mount(NULL, "/", NULL, MS_REC | MS_PRIVATE, NULL) < 0)
        return -1;

    return 0;
}
