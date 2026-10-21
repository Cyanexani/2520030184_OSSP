#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_ARGS 64
#define DELIMS " \t\r\n"

int main(void) {
    char line[1024];
    char *args[MAX_ARGS];

    while (1) {
        printf("launch> ");
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
        if (strcmp(args[0], "exit") == 0)
            break;

        for (int i = 0; i < n; i++)
            printf("argv[%d] = %s\n", i, args[i]);
        fflush(stdout);

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            continue;
        }
        if (pid == 0) {
            printf("child pid %d, parent pid %d\n", getpid(), getppid());
            fflush(stdout);
            execvp(args[0], args);
            if (errno == ENOENT) {
                fprintf(stderr, "%s: command not found\n", args[0]);
                _exit(127);
            }
            if (errno == EACCES) {
                fprintf(stderr, "%s: permission denied\n", args[0]);
                _exit(126);
            }
            perror(args[0]);
            _exit(1);
        }

        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status))
            printf("[pid %d] exited with status %d\n", pid, WEXITSTATUS(status));
        else if (WIFSIGNALED(status))
            printf("[pid %d] killed by signal %s\n", pid, strsignal(WTERMSIG(status)));
    }
    return 0;
}
