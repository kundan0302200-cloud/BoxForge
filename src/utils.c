#define _GNU_SOURCE
#include "boxforge.h"
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

void bf_die(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

void bf_log(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "[boxforge] ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
}

int parse_size_bytes(const char *s, unsigned long long *out) {
    if (!s || !*s || !out) return -1;
    char *end = NULL;
    errno = 0;
    unsigned long long n = strtoull(s, &end, 10);
    if (errno || end == s) return -1;
    unsigned long long mul = 1;
    if (*end) {
        if (!strcasecmp(end, "K") || !strcasecmp(end, "KB")) mul = 1024ULL;
        else if (!strcasecmp(end, "M") || !strcasecmp(end, "MB")) mul = 1024ULL * 1024ULL;
        else if (!strcasecmp(end, "G") || !strcasecmp(end, "GB")) mul = 1024ULL * 1024ULL * 1024ULL;
        else return -1;
    }
    if (n > ULLONG_MAX / mul) return -1;
    *out = n * mul;
    return 0;
}

int parse_cpu_percent(const char *s, unsigned long long *quota, unsigned long long *period) {
    if (!s || !quota || !period) return -1;
    char *end = NULL;
    errno = 0;
    double pct = strtod(s, &end);
    if (errno || end == s || (*end && strcmp(end, "%") != 0) || pct <= 0 || pct > 10000) return -1;
    *period = 100000;
    *quota = (unsigned long long)(pct * (double)*period / 100.0);
    if (*quota < 1) *quota = 1;
    return 0;
}
