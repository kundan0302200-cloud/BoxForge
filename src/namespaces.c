#define _GNU_SOURCE
#include "boxforge.h"
#include <sched.h>
#include <sys/mount.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

int namespace_clone_flags(void) {
    return CLONE_NEWPID | CLONE_NEWNS | CLONE_NEWUTS | CLONE_NEWIPC | CLONE_NEWNET | SIGCHLD;
}

int namespace_setup(ContainerConfig *cfg) {
    if (sethostname(cfg->hostname ? cfg->hostname : "boxforge", cfg->hostname ? strlen(cfg->hostname) : 8) < 0)
        return -1;

    /* Prevent mount changes in this namespace from propagating to the host. */
    if (mount(NULL, "/", NULL, MS_REC | MS_PRIVATE, NULL) < 0)
        return -1;

    return 0;
}
