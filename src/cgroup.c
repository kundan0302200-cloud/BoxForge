#include "boxforge.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define CGROUP_ROOT "/sys/fs/cgroup"

static int write_text(const char *path, const char *text) {
    int fd = open(path, O_WRONLY | O_CLOEXEC);
    if (fd < 0) return -1;
    size_t n = strlen(text);
    ssize_t w = write(fd, text, n);
    int saved = errno;
    close(fd);
    errno = saved;
    return w == (ssize_t)n ? 0 : (w < 0 ? saved : EIO);
}

static int path_for(char *out, size_t n, const char *id, const char *file) {
    int rc = snprintf(out, n, "%s/boxforge-%s/%s", CGROUP_ROOT, id, file);
    return (rc >= 0 && (size_t)rc < n) ? 0 : -1;
}

int cgroup_create(const char *id, const char *memory, const char *cpu, pid_t pid) {
    char dir[PATH_MAX], path[PATH_MAX], val[128];
    int rc = snprintf(dir, sizeof(dir), "%s/boxforge-%s", CGROUP_ROOT, id);
    if (rc < 0 || (size_t)rc >= sizeof(dir)) { errno = ENAMETOOLONG; return -1; }

    if (mkdir(dir, 0755) < 0 && errno != EEXIST) return -1;

    if (memory) {
        unsigned long long bytes;
        if (parse_size_bytes(memory, &bytes) < 0) { errno = EINVAL; goto fail; }
        if (path_for(path, sizeof(path), id, "memory.max") < 0) { errno = ENAMETOOLONG; goto fail; }
        snprintf(val, sizeof(val), "%llu", bytes);
        if (write_text(path, val) != 0) goto fail;
    }

    if (cpu) {
        unsigned long long quota, period;
        if (parse_cpu_percent(cpu, &quota, &period) < 0) { errno = EINVAL; goto fail; }
        if (path_for(path, sizeof(path), id, "cpu.max") < 0) { errno = ENAMETOOLONG; goto fail; }
        snprintf(val, sizeof(val), "%llu %llu", quota, period);
        if (write_text(path, val) != 0) goto fail;
    }

    if (path_for(path, sizeof(path), id, "cgroup.procs") < 0) { errno = ENAMETOOLONG; goto fail; }
    snprintf(val, sizeof(val), "%d", pid);
    if (write_text(path, val) != 0) goto fail;
    return 0;

fail: {
    int saved = errno;
    rmdir(dir);
    errno = saved;
    return -1;
}
}

int cgroup_remove(const char *id) {
    char dir[PATH_MAX];
    int rc = snprintf(dir, sizeof(dir), "%s/boxforge-%s", CGROUP_ROOT, id);
    if (rc < 0 || (size_t)rc >= sizeof(dir)) { errno = ENAMETOOLONG; return -1; }
    if (rmdir(dir) < 0 && errno != ENOENT) return -1;
    return 0;
}
