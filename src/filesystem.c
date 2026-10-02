#include "boxforge.h"
#include <errno.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <stdio.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/syscall.h>
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

static int bind_device(const char *source, const char *target) {
    if (mount(source, target, NULL, MS_BIND, NULL) < 0) {
        bf_log("warning: unable to expose %s: %s", source, strerror(errno));
        return -1;
    }
    return 0;
}

int filesystem_setup(const char *rootfs) {
    if (!rootfs || rootfs[0] != '/') {
        errno = EINVAL;
        return -1;
    }

    if (do_pivot_root(rootfs) < 0) return -1;

    if (mkdir("/proc", 0555) < 0 && errno != EEXIST) return -1;
    if (mount("proc", "/proc", "proc", MS_NOSUID | MS_NODEV | MS_NOEXEC, "") < 0)
        return -1;

    if (mkdir("/sys", 0555) < 0 && errno != EEXIST) return -1;
    if (mount("sysfs", "/sys", "sysfs",
              MS_NOSUID | MS_NODEV | MS_NOEXEC | MS_RDONLY, "") < 0)
        bf_log("warning: /sys mount unavailable: %s", strerror(errno));

    /*
     * Never bind the complete host /dev tree into the container.
     * Expose only a minimal set of character devices needed by basic Unix tools.
     */
    if (mkdir("/dev", 0755) < 0 && errno != EEXIST) return -1;
    bind_device("/dev/null", "/dev/null");
    bind_device("/dev/zero", "/dev/zero");
    bind_device("/dev/random", "/dev/random");
    bind_device("/dev/urandom", "/dev/urandom");
    bind_device("/dev/tty", "/dev/tty");

    if (mkdir("/tmp", 01777) < 0 && errno != EEXIST) return -1;
    return 0;
}
