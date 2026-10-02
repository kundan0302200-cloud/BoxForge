#define _GNU_SOURCE
#include "boxforge.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#define MAX_INPUT 1024
#define MAX_ARGS  64

static void banner(void) {
    printf("=============================================\n");
    printf("                 BOXFORGE\n");
    printf("        Minimal Linux Container Runtime\n");
    printf("=============================================\n");
    printf("Type 'help' to see available commands.\n\n");
}

static void usage(const char *p) {
    fprintf(stderr,
        "Usage:\n"
        "  %s run [--memory LIMIT] [--cpu PERCENT] [--hostname NAME] "
        "<rootfs> <command> [args...]\n"
        "\n"
        "Examples:\n"
        "  sudo %s run ./rootfs/busybox /bin/sh\n"
        "  sudo %s run --memory 100M --cpu 50%% ./rootfs/busybox /bin/sh\n",
        p, p, p);
}

static void help(void) {
    printf("\n");
    printf("Built-in Commands:\n");
    printf("  run       Start a container\n");
    printf("  help      Show available commands\n");
    printf("  clear     Clear terminal\n");
    printf("  version   Show BoxForge version\n");
    printf("  exit      Exit BoxForge\n");
    printf("\n");
    printf("Run syntax:\n");
    printf("  run [--memory LIMIT] [--cpu PERCENT] [--hostname NAME] "
           "<rootfs> <command> [args...]\n");
    printf("\n");
    printf("Examples:\n");
    printf("  run ./rootfs/busybox /bin/sh\n");
    printf("  run --memory 100M ./rootfs/busybox /bin/sh\n");
    printf("  run --cpu 50%% ./rootfs/busybox /bin/sh\n");
    printf("  run --memory 100M --cpu 50%% ./rootfs/busybox /bin/sh\n");
    printf("\n");
}

static void version(void) {
    printf("BoxForge v1.0\n");
    printf("Minimal Linux Container Runtime\n");
}

static void make_id(char *out, size_t n) {
    snprintf(out, n, "%ld-%ld", (long)getpid(), (long)time(NULL));
}

/*
 * Executes:
 *   run [options] <rootfs> <command> [args...]
 */
static int execute_run(int argc, char **argv) {
    ContainerConfig cfg = {0};

    cfg.hostname = "boxforge";
    make_id(cfg.id, sizeof(cfg.id));

    int i = 1;

    while (i < argc) {
        if (!strcmp(argv[i], "--memory") && i + 1 < argc) {
            cfg.memory_limit = argv[++i];
        } else if (!strcmp(argv[i], "--cpu") && i + 1 < argc) {
            cfg.cpu_limit = argv[++i];
        } else if (!strcmp(argv[i], "--hostname") && i + 1 < argc) {
            cfg.hostname = argv[++i];
        } else {
            break;
        }
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

    cfg.argv = &argv[i + 1];

    if (geteuid() != 0) {
        fprintf(stderr,
                "boxforge: run requires root/CAP_SYS_ADMIN on this build.\n");
        free((void *)cfg.rootfs);
        return 1;
    }

    int rc = container_run(&cfg);

    free((void *)cfg.rootfs);

    return rc < 0 ? 1 : rc;
}

/*
 * Interactive BoxForge command shell.
 */
static void interactive_shell(void) {
    char input[MAX_INPUT];

    banner();

    while (1) {
        printf("boxforge$ ");
        fflush(stdout);

        if (!fgets(input, sizeof(input), stdin)) {
            printf("\n");
            break;
        }

        input[strcspn(input, "\n")] = '\0';

        if (input[0] == '\0')
            continue;

        char *args[MAX_ARGS];
        int argc = 0;

        char *saveptr = NULL;
        char *token = strtok_r(input, " \t", &saveptr);

        while (token && argc < MAX_ARGS - 1) {
            args[argc++] = token;
            token = strtok_r(NULL, " \t", &saveptr);
        }

        args[argc] = NULL;

        if (argc == 0)
            continue;

        if (!strcmp(args[0], "help")) {
            help();
        }
        else if (!strcmp(args[0], "version")) {
            version();
        }
        else if (!strcmp(args[0], "clear")) {
            printf("\033[2J\033[H");
        }
        else if (!strcmp(args[0], "exit") ||
                 !strcmp(args[0], "quit")) {
            printf("Exiting BoxForge...\n");
            break;
        }
        else if (!strcmp(args[0], "run")) {
            int rc = execute_run(argc, args);

            if (rc != 0) {
                printf("Container exited with status %d\n", rc);
            }
        }
        else {
            printf("boxforge: command '%s' not found.\n", args[0]);
            printf("Type 'help' for available commands.\n");
        }
    }
}

int main(int argc, char **argv) {

    /*
     * No arguments:
     * start interactive BoxForge shell.
     *
     * Example:
     *   sudo ./boxforge
     */
    if (argc == 1) {
        if (geteuid() != 0) {
            printf("BoxForge interactive mode started.\n");
            printf("Use 'sudo ./boxforge' to run containers.\n\n");
        }

        interactive_shell();
        return 0;
    }

    /*
     * Direct command-line mode:
     *
     *   sudo ./boxforge run ./rootfs/busybox /bin/sh
     */
    if (strcmp(argv[1], "run") != 0) {
        fprintf(stderr,
                "boxforge: command '%s' is not implemented.\n\n",
                argv[1]);
        usage(argv[0]);
        return 2;
    }

    return execute_run(argc - 1, &argv[1]);
}
