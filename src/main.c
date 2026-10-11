#include "boxforge.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define BF_VERSION "1.1"
#define BF_PROMPT "boxforge"
#define COLOR_RESET "\033[0m"
#define COLOR_BOLD  "\033[1m"
#define COLOR_DIM   "\033[2m"
#define COLOR_CYAN  "\033[36m"
#define COLOR_BLUE  "\033[94m"
#define COLOR_GREEN "\033[32m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_RED   "\033[31m"

static int use_color;

static const char *paint(const char *code) {
    return use_color ? code : "";
}

static void init_terminal(void) {
    const char *no_color = getenv("NO_COLOR");
    const char *term = getenv("TERM");
    use_color = isatty(STDOUT_FILENO) && (!no_color || !*no_color) &&
                (!term || strcmp(term, "dumb") != 0);
}

static void rule(void) {
    printf("%s────────────────────────────────────────────────────────%s\n",
           paint(COLOR_DIM), paint(COLOR_RESET));
}

static void banner(void) {
    printf("\n%s%s  ▄▄▄▄  ▒█████  ▒██   ██▒  █████▒▒█████  ▄████▄  ▓█████%s\n",
           paint(COLOR_BOLD), paint(COLOR_BLUE), paint(COLOR_RESET));
    printf("%s  ▓█████▄▒██▒  ██▒▒▒ █ █ ▒░▓██   ▒▒██▒  ██▒▒██▀ ▀█  ▓█   ▀%s\n",
           paint(COLOR_BLUE), paint(COLOR_RESET));
    printf("%s  ▒██▒ ▄██▒██░  ██▒░░  █   ░▒████ ░▒██░  ██▒▒▓█    ▄▒███%s\n",
           paint(COLOR_BLUE), paint(COLOR_RESET));
    printf("%s  ▒██░█▀  ▒██   ██░ ░ █ █ ▒ ░▓█▒  ░▒██   ██░▒▓▓▄ ▄██▒▓█  ▄%s\n",
           paint(COLOR_BLUE), paint(COLOR_RESET));
    printf("%s  ░▓█  ▀█▓░ ████▓▒░▒██▒ ▒██▒░▒█░   ░ ████▓▒░▒ ▓███▀ ░▒████▒%s\n",
           paint(COLOR_BLUE), paint(COLOR_RESET));
    printf("%s  ░▒▓███▀▒░ ▒░▒░▒░ ▒▒ ░ ░▓ ░ ▒ ░   ░ ▒░▒░▒░ ░ ░▒ ▒  ░░ ▒░ ░%s\n",
           paint(COLOR_BLUE), paint(COLOR_RESET));
    printf("\n");
    printf("  %sMinimal Linux Container Runtime%s  %s·%s  %sv%s%s\n",
           paint(COLOR_DIM), paint(COLOR_RESET), paint(COLOR_DIM),
           paint(COLOR_RESET), paint(COLOR_CYAN), BF_VERSION, paint(COLOR_RESET));
    rule();
    printf("  Type %shelp%s to see commands, or %sexit%s to leave.\n\n",
           paint(COLOR_BOLD), paint(COLOR_RESET),
           paint(COLOR_BOLD), paint(COLOR_RESET));
}

static void usage(const char *program) {
    printf("%sUSAGE%s\n", paint(COLOR_BOLD), paint(COLOR_RESET));
    printf("  %s run [OPTIONS] <rootfs> <command> [args...]\n", program);
    printf("  %s [--help | --version]\n\n", program);
    printf("%sOPTIONS%s\n", paint(COLOR_BOLD), paint(COLOR_RESET));
    printf("  --memory LIMIT     Set cgroup v2 memory limit (e.g. 64M, 1G)\n");
    printf("  --cpu PERCENT      Set CPU quota (e.g. 50, 100, 200)\n");
    printf("  --hostname NAME    Set the container hostname\n");
    printf("  --userns           Enable user namespace UID/GID mapping\n\n");
    printf("%sEXAMPLES%s\n", paint(COLOR_BOLD), paint(COLOR_RESET));
    printf("  sudo %s run ./rootfs/busybox /bin/sh\n", program);
    printf("  sudo %s run --memory 128M --cpu 50 ./rootfs/busybox /bin/sh\n", program);
    printf("  sudo %s run --hostname lab-box ./rootfs/busybox /bin/hostname\n", program);
}

static void help(void) {
    printf("\n%sCOMMANDS%s\n", paint(COLOR_BOLD), paint(COLOR_RESET));
    printf("  %srun%s       Launch an isolated container\n", paint(COLOR_CYAN), paint(COLOR_RESET));
    printf("  %sstatus%s    Show runtime and host capabilities\n", paint(COLOR_CYAN), paint(COLOR_RESET));
    printf("  %shelp%s      Show this command reference\n", paint(COLOR_CYAN), paint(COLOR_RESET));
    printf("  %sversion%s   Show build version\n", paint(COLOR_CYAN), paint(COLOR_RESET));
    printf("  %sclear%s     Clear the terminal\n", paint(COLOR_CYAN), paint(COLOR_RESET));
    printf("  %sexit%s      Exit the interactive CLI\n\n", paint(COLOR_CYAN), paint(COLOR_RESET));
    printf("%sRUN SYNTAX%s\n", paint(COLOR_BOLD), paint(COLOR_RESET));
    printf("  run [--memory LIMIT] [--cpu PERCENT] [--hostname NAME] [--userns] ");
    printf("<rootfs> <command> [args...]\n\n");
    printf("%sTIP%s  Paths and commands are accepted as separate arguments.\n\n",
           paint(COLOR_YELLOW), paint(COLOR_RESET));
}

static void version(void) {
    printf("BoxForge %s\n", BF_VERSION);
    printf("Minimal Linux Container Runtime\n");
}

static void status(void) {
    struct stat st;
    int cgroup_v2 = stat("/sys/fs/cgroup/cgroup.controllers", &st) == 0;
    int rootfs = stat("./rootfs/busybox", &st) == 0 && S_ISDIR(st.st_mode);
    int binary = access("./boxforge", X_OK) == 0;

    printf("\n%sRUNTIME STATUS%s\n", paint(COLOR_BOLD), paint(COLOR_RESET));
    rule();
    printf("  Privileges       %s\n", geteuid() == 0 ? "root (enabled)" : "unprivileged (container launch needs sudo)");
    printf("  cgroups v2       %s%s%s\n", cgroup_v2 ? paint(COLOR_GREEN) : paint(COLOR_RED),
           cgroup_v2 ? "available" : "not detected", paint(COLOR_RESET));
    printf("  BusyBox rootfs   %s%s%s\n", rootfs ? paint(COLOR_GREEN) : paint(COLOR_YELLOW),
           rootfs ? "found: ./rootfs/busybox" : "not found at ./rootfs/busybox",
           paint(COLOR_RESET));
    printf("  BoxForge binary  %s%s%s\n", binary ? paint(COLOR_GREEN) : paint(COLOR_RED),
           binary ? "available in current directory" : "not found; run make",
           paint(COLOR_RESET));
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
        else if (!strncmp(argv[i], "--", 2)) {
            fprintf(stderr, "%serror:%s unknown or incomplete option '%s'\n",
                    paint(COLOR_RED), paint(COLOR_RESET), argv[i]);
            return 2;
        } else
            break;
        i++;
    }

    if (argc - i < 2) {
        fprintf(stderr, "%serror:%s expected a rootfs and command\n",
                paint(COLOR_RED), paint(COLOR_RESET));
        printf("  Usage: run [OPTIONS] <rootfs> <command> [args...]\n");
        return 2;
    }

    cfg.rootfs = realpath(argv[i], NULL);
    if (!cfg.rootfs) {
        fprintf(stderr, "%serror:%s cannot resolve rootfs '%s': %s\n",
                paint(COLOR_RED), paint(COLOR_RESET), argv[i], strerror(errno));
        return 1;
    }

    if (!strcmp(cfg.rootfs, "/") ||
        stat(cfg.rootfs, &st) < 0 || !S_ISDIR(st.st_mode)) {
        fprintf(stderr, "%serror:%s rootfs must be a directory other than /\n",
                paint(COLOR_RED), paint(COLOR_RESET));
        free((void *)cfg.rootfs);
        return 1;
    }

    cfg.argv = &argv[i + 1];

    if (geteuid() != 0) {
        fprintf(stderr, "%serror:%s container launch requires root/CAP_SYS_ADMIN on this build\n",
                paint(COLOR_RED), paint(COLOR_RESET));
        fprintf(stderr, "  Try: sudo ./boxforge run %s %s\n", argv[i], argv[i + 1]);
        free((void *)cfg.rootfs);
        return 1;
    }

    printf("%s→%s Preparing container %s%s%s\n",
           paint(COLOR_CYAN), paint(COLOR_RESET),
           paint(COLOR_BOLD), cfg.id, paint(COLOR_RESET));
    printf("  rootfs     %s\n", cfg.rootfs);
    printf("  hostname   %s\n", cfg.hostname);
    printf("  memory     %s\n", cfg.memory_limit ? cfg.memory_limit : "unlimited");
    printf("  cpu quota  %s%s\n\n", cfg.cpu_limit ? cfg.cpu_limit : "unlimited",
           cfg.cpu_limit ? "%" : "");

    int rc = container_run(&cfg);
    free((void *)cfg.rootfs);
    if (rc < 0) {
        fprintf(stderr, "%s✗%s Container launch failed. Check the error above.\n",
                paint(COLOR_RED), paint(COLOR_RESET));
        return 1;
    }
    if (rc == 0)
        printf("\n%s✓%s Container exited successfully.\n", paint(COLOR_GREEN), paint(COLOR_RESET));
    else
        printf("\n%s!%s Container exited with status %d.\n",
               paint(COLOR_YELLOW), paint(COLOR_RESET), rc);
    return rc;
}

/* Small shell-like tokenizer: supports quoted arguments and backslash escapes.
 * It deliberately does not expand variables, pipes, redirects, or substitutions. */
static int tokenize(char *input, char **args, int max_args) {
    char *src = input, *dst = input;
    int argc = 0;

    while (*src) {
        while (*src == ' ' || *src == '\t' || *src == '\r' || *src == '\n')
            src++;
        if (!*src) break;
        if (argc >= max_args - 1) { errno = E2BIG; return -1; }
        args[argc++] = dst;
        char quote = 0;
        while (*src) {
            char ch = *src++;
            if (!quote && (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n'))
                break;
            if (!quote && (ch == '\'' || ch == '"')) { quote = ch; continue; }
            if (quote && ch == quote) { quote = 0; continue; }
            if (ch == '\\' && *src) ch = *src++;
            *dst++ = ch;
        }
        if (quote) { errno = EINVAL; return -1; }
        *dst++ = '\0';
    }
    args[argc] = NULL;
    return argc;
}

static void interactive_shell(void) {
    char input[BF_MAX_INPUT];
    banner();

    while (1) {
        printf("%s%s%s %s›%s ", paint(COLOR_BOLD), BF_PROMPT,
               paint(COLOR_RESET), paint(COLOR_CYAN), paint(COLOR_RESET));
        fflush(stdout);

        if (!fgets(input, sizeof(input), stdin)) {
            printf("\n");
            break;
        }
        if (!strchr(input, '\n') && !feof(stdin)) {
            int ch;
            while ((ch = getchar()) != '\n' && ch != EOF) {}
            fprintf(stderr, "%serror:%s input line too long (max %d bytes)\n",
                    paint(COLOR_RED), paint(COLOR_RESET), BF_MAX_INPUT - 1);
            continue;
        }

        char *args[BF_MAX_ARGS];
        int argc = tokenize(input, args, BF_MAX_ARGS);
        if (argc == 0) continue;
        if (argc < 0) {
            fprintf(stderr, "%serror:%s %s\n", paint(COLOR_RED), paint(COLOR_RESET),
                    errno == E2BIG ? "too many arguments" : "unmatched quote in command");
            continue;
        }

        if (!strcmp(args[0], "help") || !strcmp(args[0], "?")) help();
        else if (!strcmp(args[0], "version") || !strcmp(args[0], "--version")) version();
        else if (!strcmp(args[0], "status")) status();
        else if (!strcmp(args[0], "clear")) printf("\033[2J\033[H");
        else if (!strcmp(args[0], "exit") || !strcmp(args[0], "quit")) {
            printf("%s✓%s Bye.\n", paint(COLOR_GREEN), paint(COLOR_RESET));
            break;
        } else if (!strcmp(args[0], "run")) {
            int rc = execute_run(argc, args);
            if (rc != 0) printf("%sCommand failed (status %d).%s\n",
                                paint(COLOR_RED), rc, paint(COLOR_RESET));
        } else {
            fprintf(stderr, "%sunknown command:%s %s\n",
                    paint(COLOR_RED), paint(COLOR_RESET), args[0]);
            fprintf(stderr, "  Type '%shelp%s' to see available commands.\n",
                    paint(COLOR_CYAN), paint(COLOR_RESET));
        }
    }
}

int main(int argc, char **argv) {
    init_terminal();

    if (argc == 1) {
        if (geteuid() != 0) {
            printf("%snote:%s launch containers with sudo on this build.\n\n",
                   paint(COLOR_YELLOW), paint(COLOR_RESET));
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
        fprintf(stderr, "%serror:%s command '%s' is not implemented\n\n",
                paint(COLOR_RED), paint(COLOR_RESET), argv[1]);
        usage(argv[0]);
        return 2;
    }

    return execute_run(argc - 1, &argv[1]);
}
