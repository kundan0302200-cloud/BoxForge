#include "boxforge.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

#define BF_RESET "\033[0m"
#define BF_BOLD "\033[1m"
#define BF_DIM "\033[2m"
#define BF_CYAN "\033[36m"
#define BF_BLUE "\033[94m"
#define BF_GREEN "\033[32m"
#define BF_YELLOW "\033[33m"
#define BF_RED "\033[31m"
static int use_color;
static const char *paint(const char *s) { return use_color ? s : ""; }
static void init_terminal(void) {
    const char *no_color = getenv("NO_COLOR");
    const char *term = getenv("TERM");
    use_color = isatty(STDOUT_FILENO) && (!no_color || !*no_color) && (!term || strcmp(term, "dumb"));
}
static void rule(void) { printf("%s────────────────────────────────────────────────────────%s\n", paint(BF_DIM), paint(BF_RESET)); }


static void banner(void) {
    printf("\n%s%sBOXFORGE%s %s/ Minimal Linux Container Runtime%s\n",
           paint(BF_BOLD), paint(BF_BLUE), paint(BF_RESET), paint(BF_DIM), paint(BF_RESET));
    printf("  %sContainer isolation from the terminal.%s  %sv%s\n",
           paint(BF_DIM), paint(BF_RESET), paint(BF_CYAN), "1.1");
    rule();
    printf("  Type %shelp%s for commands, %sstatus%s for runtime checks, or %sexit%s to leave.\n\n",
           paint(BF_BOLD), paint(BF_RESET), paint(BF_BOLD), paint(BF_RESET), paint(BF_BOLD), paint(BF_RESET));
}

static void usage(const char *p) {
    fprintf(stderr,
        "Usage:\n"
        "  %s run [--memory LIMIT] [--cpu PERCENT] [--hostname NAME] [--userns] "
        "<rootfs> <command> [args...]\n"
        "  %s --help\n"
        "  %s --version\n"
        "\n"
        "Examples:\n"
        "  sudo %s run ./rootfs/busybox /bin/sh\n"
        "  sudo %s run --memory 100M --cpu 50%% ./rootfs/busybox /bin/sh\n"
        "  sudo %s run --userns ./rootfs/busybox /bin/sh\n",
        p, p, p, p, p, p);
}

static void help(void) {
    printf("\n%sCOMMANDS%s\n", paint(BF_BOLD), paint(BF_RESET));
    printf("  %srun%s       Launch an isolated container\n", paint(BF_CYAN), paint(BF_RESET));
    printf("  %sstatus%s    Show runtime and host capabilities\n", paint(BF_CYAN), paint(BF_RESET));
    printf("  %shelp%s      Show command reference\n", paint(BF_CYAN), paint(BF_RESET));
    printf("  %sversion%s   Show BoxForge version\n", paint(BF_CYAN), paint(BF_RESET));
    printf("  %sclear%s     Clear the terminal\n", paint(BF_CYAN), paint(BF_RESET));
    printf("  %sexit%s      Exit the CLI\n\n", paint(BF_CYAN), paint(BF_RESET));
    printf("%sRUN SYNTAX%s\n", paint(BF_BOLD), paint(BF_RESET));
    printf("  run [--memory LIMIT] [--cpu PERCENT] [--hostname NAME] [--userns] <rootfs> <command> [args...]\n\n");
}

static void version(void) {
    printf("BoxForge v1.1\n");
    printf("Minimal Linux Container Runtime\n");
}

static void status(void) {
    struct stat st;
    int cg = stat("/sys/fs/cgroup/cgroup.controllers", &st) == 0;
    int rootfs = stat("./rootfs/busybox", &st) == 0 && S_ISDIR(st.st_mode);
    int binary = access("./boxforge", X_OK) == 0;
    printf("\n%sRUNTIME STATUS%s\n", paint(BF_BOLD), paint(BF_RESET));
    rule();
    printf("  Privileges       %s\n", geteuid() == 0 ? "root" : "unprivileged (use sudo to launch)");
    printf("  cgroups v2       %s%s%s\n", paint(cg ? BF_GREEN : BF_RED), cg ? "available" : "not detected", paint(BF_RESET));
    printf("  BusyBox rootfs   %s%s%s\n", paint(rootfs ? BF_GREEN : BF_YELLOW), rootfs ? "found" : "not found at ./rootfs/busybox", paint(BF_RESET));
    printf("  Binary           %s%s%s\n", paint(binary ? BF_GREEN : BF_RED), binary ? "available" : "run make to build", paint(BF_RESET));
    rule();
    printf("\n");
}

static void make_id(char *out, size_t n) {
    snprintf(out, n, "%ld-%ld", (long)getpid(), (long)time(NULL));
}

static int execute_run(int argc, char **argv) {
    ContainerConfig cfg = {0};
    struct stat st;

    cfg.hostname = "boxforge";
    cfg.host_uid = getuid();
    cfg.host_gid = getgid();
    make_id(cfg.id, sizeof(cfg.id));

    int i = 1;
    while (i < argc) {
        if (!strcmp(argv[i], "--memory") && i + 1 < argc)
            cfg.memory_limit = argv[++i];
        else if (!strcmp(argv[i], "--cpu") && i + 1 < argc)
            cfg.cpu_limit = argv[++i];
        else if (!strcmp(argv[i], "--hostname") && i + 1 < argc)
            cfg.hostname = argv[++i];
        else if (!strcmp(argv[i], "--userns"))
            cfg.use_userns = 1;
        else
            break;
        i++;
    }

    if (argc - i < 2) {
        fprintf(stderr, "boxforge: missing rootfs or command.\n\n");
        usage("./boxforge");
        return 2;
    }

    cfg.rootfs = realpath(argv[i], NULL);
    if (!cfg.rootfs) {
        perror("rootfs");
        return 1;
    }

    if (!strcmp(cfg.rootfs, "/") ||
        stat(cfg.rootfs, &st) < 0 || !S_ISDIR(st.st_mode)) {
        fprintf(stderr, "boxforge: rootfs must be a directory other than /.\n");
        free((void *)cfg.rootfs);
        return 1;
    }

    cfg.argv = &argv[i + 1];

    if (geteuid() != 0) {
        fprintf(stderr, "boxforge: run requires root/CAP_SYS_ADMIN on this build.\n");
        free((void *)cfg.rootfs);
        return 1;
    }

    int rc = container_run(&cfg);
    free((void *)cfg.rootfs);
    return rc < 0 ? 1 : rc;
}

static void interactive_shell(void) {
    char input[BF_MAX_INPUT];
    banner();

    while (1) {
        printf("%sboxforge%s %s›%s ", paint(BF_BOLD), paint(BF_RESET), paint(BF_CYAN), paint(BF_RESET));
        fflush(stdout);
        if (!fgets(input, sizeof(input), stdin)) {
            printf("\n");
            break;
        }

        input[strcspn(input, "\n")] = '\0';
        if (input[0] == '\0') continue;

        char *args[BF_MAX_ARGS];
        int argc = 0;
        char *saveptr = NULL;
        char *token = strtok_r(input, " \t", &saveptr);
        while (token && argc < BF_MAX_ARGS - 1) {
            args[argc++] = token;
            token = strtok_r(NULL, " \t", &saveptr);
        }
        args[argc] = NULL;

        if (!strcmp(args[0], "help")) help();
        else if (!strcmp(args[0], "version")) version();
        else if (!strcmp(args[0], "status")) status();
        else if (!strcmp(args[0], "clear")) printf("\033[2J\033[H");
        else if (!strcmp(args[0], "exit") || !strcmp(args[0], "quit")) {
            printf("%s✓%s Bye.\n", paint(BF_GREEN), paint(BF_RESET));
            break;
        } else if (!strcmp(args[0], "run")) {
            int rc = execute_run(argc, args);
            if (rc != 0) printf("Container exited with status %d\n", rc);
        } else {
            printf("boxforge: command '%s' not found.\n", args[0]);
            printf("Type 'help' for available commands.\n");
        }
    }
}

int main(int argc, char **argv) {
    init_terminal();
    if (argc == 1) {
        if (geteuid() != 0) {
            printf("BoxForge interactive mode started.\n");
            printf("Use 'sudo ./boxforge' to run containers.\n\n");
        }
        interactive_shell();
        return 0;
    }

    if (!strcmp(argv[1], "--version") || !strcmp(argv[1], "-v") ||
        !strcmp(argv[1], "version")) {
        version();
        return 0;
    }

    if (!strcmp(argv[1], "--help") || !strcmp(argv[1], "-h") ||
        !strcmp(argv[1], "help")) {
        usage(argv[0]);
        return 0;
    }

    if (strcmp(argv[1], "run") != 0) {
        fprintf(stderr, "boxforge: command '%s' is not implemented.\n\n", argv[1]);
        usage(argv[0]);
        return 2;
    }

    return execute_run(argc - 1, &argv[1]);
}
