#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_ARGS 64

int main(int argc, char *argv[]) {
    char line[1024];
    char *args[MAX_ARGS];
    char **cmd = argv + 1;

    if (argc < 2) {
        printf("Enter a command: ");
        fflush(stdout);
        if (fgets(line, sizeof(line), stdin) == NULL)
            return 0;
        int n = 0;
        char *tok = strtok(line, " \t\n");
        while (tok != NULL && n < MAX_ARGS - 1) {
            args[n++] = tok;
            tok = strtok(NULL, " \t\n");
        }
        args[n] = NULL;
        if (n == 0) {
            printf("No command given\n");
            return 1;
        }
        cmd = args;
    }

    printf("Executing:");
    for (int i = 0; cmd[i] != NULL; i++)
        printf(" %s", cmd[i]);
    printf("\n\n");
    fflush(stdout);

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    if (pid == 0) {
        execvp(cmd[0], cmd);
        perror(cmd[0]);
        _exit(127);
    }

    int status;
    waitpid(pid, &status, 0);
    clock_gettime(CLOCK_MONOTONIC, &end);
    double ms = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_nsec - start.tv_nsec) / 1e6;

    if (WIFEXITED(status))
        printf("\nPID %d exited with status %d after %.2f ms\n", pid, WEXITSTATUS(status), ms);
    else if (WIFSIGNALED(status))
        printf("\nPID %d was killed by %s after %.2f ms\n", pid, strsignal(WTERMSIG(status)), ms);
    return 0;
}
