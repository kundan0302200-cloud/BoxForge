#define _GNU_SOURCE
#include "boxforge.h"
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static volatile sig_atomic_t terminate_requested = 0;
static void handle_term(int sig) { (void)sig; terminate_requested = 1; }

int lifecycle_init(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_term;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGTERM, &sa, NULL) < 0) return -1;
    if (sigaction(SIGINT, &sa, NULL) < 0) return -1;
    return 0;
}

int lifecycle_wait(pid_t pid, int *status) {
    while (1) {
        pid_t r = waitpid(pid, status, 0);
        if (r == pid) return 0;
        if (r < 0 && errno == EINTR) continue;
        return -1;
    }
}
