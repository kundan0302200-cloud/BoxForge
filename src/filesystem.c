#include "boxforge.h"
#include <errno.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <stdio.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <unistd.h>

static int ensure_dir(const char *p) {
    if (mkdir(p, 0755) < 0 && errno != EEXIST) return -1;
    return 0;
}

static int do_pivot_root(const char *newroot) {
    char oldroot[PATH_MAX];
    int n = snprintf(oldroot, sizeof(oldroot), "%s/.oldroot", newroot);
    if (n < 0 || (size_t)n >= sizeof(oldroot)) { errno = ENAMETOOLONG; return -1; }
    if (ensure_dir(oldroot) < 0) return -1;
    if (mount(newroot, newroot, NULL, MS_BIND | MS_REC, NULL) < 0) return -1;
    if (syscall(SYS_pivot_root, newroot, oldroot) < 0) return -1;
    if (chdir("/") < 0) return -1;
    if (umount2("/.oldroot", MNT_DETACH) < 0) return -1;
    if (rmdir("/.oldroot") < 0) return -1;
    return 0;
}

static int ensure_file(const char *path, mode_t mode) {
    int fd = open(path, O_CREAT | O_RDONLY | O_CLOEXEC, mode);
    if (fd < 0) return -1;
    close(fd);
    return 0;
}

/* Bind only selected host device nodes into the new rootfs before pivot_root.
 * Doing this after pivot_root would resolve the source paths inside the container. */
static int bind_device(const char *rootfs, const char *name) {
    char source[PATH_MAX], target[PATH_MAX];
    int a = snprintf(source, sizeof(source), "/dev/%s", name);
    int b = snprintf(target, sizeof(target), "%s/dev/%s", rootfs, name);
    if (a < 0 || (size_t)a >= sizeof(source) ||
        b < 0 || (size_t)b >= sizeof(target)) {
        errno = ENAMETOOLONG;
        return -1;
    }
    if (ensure_file(target, 0666) < 0) return -1;
    if (mount(source, target, NULL, MS_BIND, NULL) < 0) return -1;
    return 0;
}

int filesystem_setup(const char *rootfs) {
    static const char *devices[] = {"null", "zero", "random", "urandom", "tty"};
    if (!rootfs || rootfs[0] != '/') { errno = EINVAL; return -1; }

    char devdir[PATH_MAX];
    int n = snprintf(devdir, sizeof(devdir), "%s/dev", rootfs);
    if (n < 0 || (size_t)n >= sizeof(devdir)) { errno = ENAMETOOLONG; return -1; }
    if (ensure_dir(devdir) < 0) return -1;

    for (size_t i = 0; i < sizeof(devices) / sizeof(devices[0]); i++) {
        if (bind_device(rootfs, devices[i]) < 0) {
            bf_log("warning: unable to expose /dev/%s in rootfs: %s",
                   devices[i], strerror(errno));
        }
    }

    if (do_pivot_root(rootfs) < 0) return -1;

    if (mkdir("/proc", 0555) < 0 && errno != EEXIST) return -1;
    if (mount("proc", "/proc", "proc", MS_NOSUID | MS_NODEV | MS_NOEXEC, "") < 0)
        return -1;

    if (mkdir("/sys", 0555) < 0 && errno != EEXIST) return -1;
    if (mount("sysfs", "/sys", "sysfs",
              MS_NOSUID | MS_NODEV | MS_NOEXEC | MS_RDONLY, "") < 0)
        bf_log("warning: /sys mount unavailable: %s", strerror(errno));

    if (mkdir("/tmp", 01777) < 0 && errno != EEXIST) return -1;
    return 0;
}
