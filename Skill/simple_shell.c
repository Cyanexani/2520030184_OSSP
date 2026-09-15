#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_TOKENS 64
#define DELIMS " \t\r\n"

static int tokenize(char *line, char **tokens) {
    int n = 0;
    char *tok = strtok(line, DELIMS);
    while (tok != NULL && n < MAX_TOKENS - 1) {
        tokens[n++] = tok;
        tok = strtok(NULL, DELIMS);
    }
    tokens[n] = NULL;
    return n;
}

int main(void) {
    char line[1024];
    char *tokens[MAX_TOKENS];

    while (1) {
        printf("myshell> ");
        fflush(stdout);

        if (fgets(line, sizeof(line), stdin) == NULL) {
            printf("\n");
            break;
        }

        int n = tokenize(line, tokens);
        if (n == 0)
            continue;

        if (strcmp(tokens[0], "exit") == 0)
            break;

        if (strcmp(tokens[0], "cd") == 0) {
            if (n < 2 || chdir(tokens[1]) != 0)
                fprintf(stderr, "cd: could not change directory\n");
            continue;
        }

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            continue;
        }

        if (pid == 0) {
            execvp(tokens[0], tokens);
            perror("exec");
            exit(EXIT_FAILURE);
        } else {
            int status;
            waitpid(pid, &status, 0);
        }
    }

    return 0;
}
