#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>

#define MAX_HISTORY 100

static char *history[MAX_HISTORY];
static int history_count = 0;

static void cleanup(void) {
    for (int i = 0; i < history_count; i++)
        free(history[i]);
    printf("cleanup: freed %d history entries\n", history_count);
}

static void on_sigint(int sig) {
    (void)sig;
}

static void print_prompt(void) {
    char *cwd = getcwd(NULL, 0);
    const char *home = getenv("HOME");
    size_t hl = home != NULL ? strlen(home) : 0;

    if (cwd == NULL)
        printf("?$ ");
    else if (hl > 0 && strncmp(cwd, home, hl) == 0 && (cwd[hl] == '/' || cwd[hl] == '\0'))
        printf("~%s$ ", cwd + hl);
    else
        printf("%s$ ", cwd);
    free(cwd);
    fflush(stdout);
}

int main(void) {
    char line[1024];
    struct sigaction sa;

    atexit(cleanup);

    /* no SA_RESTART, so Ctrl-C interrupts fgets and we can print a fresh prompt */
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_sigint;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);

    while (1) {
        print_prompt();
        if (fgets(line, sizeof(line), stdin) == NULL) {
            if (ferror(stdin) && errno == EINTR) {
                clearerr(stdin);
                printf("\n");
                continue;
            }
            printf("exit\n");
            exit(EXIT_SUCCESS);
        }
        line[strcspn(line, "\n")] = '\0';

        char *cmd = strtok(line, " \t");
        char *arg = strtok(NULL, " \t");
        if (cmd == NULL)
            continue;

        if (strcmp(cmd, "pwd") == 0) {
            /* getcwd(NULL, 0) allocates a buffer as long as the path needs */
            char *cwd = getcwd(NULL, 0);
            if (cwd == NULL) {
                perror("getcwd");
                continue;
            }
            printf("%s (%zu characters)\n", cwd, strlen(cwd));
            free(cwd);
        } else if (strcmp(cmd, "cd") == 0) {
            const char *dir = arg != NULL ? arg : getenv("HOME");
            if (dir == NULL || chdir(dir) == -1)
                perror("cd");
        } else if (strcmp(cmd, "exit") == 0) {
            if (arg == NULL)
                exit(EXIT_SUCCESS);
            char *end;
            long code = strtol(arg, &end, 10);
            if (*end != '\0') {
                printf("exit: %s: numeric argument required\n", arg);
                continue;
            }
            exit((int)code);
        } else if (history_count < MAX_HISTORY) {
            history[history_count] = strdup(cmd);
            history_count++;
            printf("%s: not a command here, stored in history (%d)\n", cmd, history_count);
        }
    }
}
