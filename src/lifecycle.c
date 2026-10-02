#include "boxforge.h"
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static volatile sig_atomic_t child_pid_for_signal = -1;

static void forward_signal(int sig) {
    pid_t pid = (pid_t)child_pid_for_signal;
    if (pid > 0)
        kill(pid, sig);
}

int lifecycle_init(void) {
    return 0;
}

int lifecycle_wait(pid_t pid, int *status) {
    child_pid_for_signal = pid;

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = forward_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGINT, &sa, NULL) < 0) return -1;
    if (sigaction(SIGTERM, &sa, NULL) < 0) return -1;

    while (1) {
        pid_t r = waitpid(pid, status, 0);
        if (r == pid) {
            child_pid_for_signal = -1;
            return 0;
        }
        if (r < 0 && errno == EINTR) continue;
        child_pid_for_signal = -1;
        return -1;
    }
}
