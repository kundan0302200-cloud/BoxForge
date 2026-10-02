#define _GNU_SOURCE
#include "boxforge.h"
#include <errno.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>

static int ensure_dir(const char *p) {
    if (mkdir(p, 0755) < 0 && errno != EEXIST) return -1;
    return 0;
}

static int do_pivot_root(const char *newroot) {
    char oldroot[PATH_MAX];
    if (snprintf(oldroot, sizeof(oldroot), "%s/.oldroot", newroot) >= (int)sizeof(oldroot)) return -1;
    if (ensure_dir(oldroot) < 0) return -1;
    if (mount(newroot, newroot, NULL, MS_BIND | MS_REC, NULL) < 0) return -1;
    if (syscall(SYS_pivot_root, newroot, oldroot) < 0) return -1;
    if (chdir("/") < 0) return -1;
    if (umount2("/.oldroot", MNT_DETACH) < 0) return -1;
    if (rmdir("/.oldroot") < 0) return -1;
    return 0;
}

int filesystem_setup(const char *rootfs) {
    if (!rootfs || rootfs[0] != '/') {
        errno = EINVAL;
        return -1;
    }

    if (do_pivot_root(rootfs) < 0) return -1;

    if (mkdir("/proc", 0555) < 0 && errno != EEXIST) return -1;
    if (mount("proc", "/proc", "proc", 0, "") < 0) return -1;

    if (mkdir("/dev", 0755) < 0 && errno != EEXIST) return -1;
    /* Minimal dev setup; a production runtime would use a more complete device policy. */
    if (mount("/dev", "/dev", NULL, MS_BIND | MS_REC, NULL) < 0) {
        /* A minimal rootfs may already contain /dev. Don't make /dev fatal. */
        bf_log("warning: /dev bind setup skipped: %s", strerror(errno));
    }
    return 0;
}
