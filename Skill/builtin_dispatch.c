#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_ARGS 64
#define DELIMS " \t\r\n"

struct builtin {
    const char *name;
    int (*fn)(char **argv);
    const char *help;
};

static int sh_cd(char **argv) {
    const char *dir = argv[1] != NULL ? argv[1] : getenv("HOME");
    if (dir == NULL || chdir(dir) != 0)
        perror("cd");
    return 1;
}

static int sh_pwd(char **argv) {
    char cwd[1024];
    (void)argv;
    if (getcwd(cwd, sizeof(cwd)) != NULL)
        printf("%s\n", cwd);
    else
        perror("pwd");
    return 1;
}

static int sh_echo(char **argv) {
    for (int i = 1; argv[i] != NULL; i++)
        printf("%s%s", i > 1 ? " " : "", argv[i]);
    printf("\n");
    return 1;
}

static int sh_exit(char **argv) {
    (void)argv;
    return 0;
}

static int sh_help(char **argv);

static struct builtin builtins[] = {
    {"cd", sh_cd, "change the shell's own working directory"},
    {"pwd", sh_pwd, "print the working directory"},
    {"echo", sh_echo, "print the arguments"},
    {"help", sh_help, "list the built-in commands"},
    {"exit", sh_exit, "leave the shell"},
};

#define NUM_BUILTINS (int)(sizeof(builtins) / sizeof(builtins[0]))

static int sh_help(char **argv) {
    (void)argv;
    for (int i = 0; i < NUM_BUILTINS; i++)
        printf("  %-5s %s\n", builtins[i].name, builtins[i].help);
    printf("Anything else is run as an external program.\n");
    return 1;
}

static void run_external(char **args) {
    printf("[external] %s\n", args[0]);
    fflush(stdout);
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return;
    }
    if (pid == 0) {
        execvp(args[0], args);
        perror(args[0]);
        _exit(127);
    }
    int status;
    waitpid(pid, &status, 0);
    printf("[external] pid %d exited with status %d\n", pid, WEXITSTATUS(status));
}

int main(void) {
    char line[1024];
    char *args[MAX_ARGS];
    int running = 1;

    while (running) {
        printf("dispatch> ");
        fflush(stdout);
        if (fgets(line, sizeof(line), stdin) == NULL) {
            printf("\n");
            break;
        }

        int n = 0;
        char *tok = strtok(line, DELIMS);
        while (tok != NULL && n < MAX_ARGS - 1) {
            args[n++] = tok;
            tok = strtok(NULL, DELIMS);
        }
        args[n] = NULL;
        if (n == 0)
            continue;

        int found = 0;
        for (int i = 0; i < NUM_BUILTINS; i++) {
            if (strcmp(args[0], builtins[i].name) == 0) {
                printf("[builtin] %s\n", args[0]);
                running = builtins[i].fn(args);
                found = 1;
                break;
            }
        }
        if (!found)
            run_external(args);
    }
    return 0;
}
