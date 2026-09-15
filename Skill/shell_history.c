#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_TOKENS 64
#define MAX_HISTORY 100
#define DELIMS " \t\r\n"

static char *history[MAX_HISTORY];
static int history_count = 0;

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

static void add_history(const char *line) {
    if (history_count < MAX_HISTORY) {
        history[history_count++] = strdup(line);
    } else {
        free(history[0]);
        memmove(history, history + 1, (MAX_HISTORY - 1) * sizeof(char *));
        history[MAX_HISTORY - 1] = strdup(line);
    }
}

static void print_history(void) {
    for (int i = 0; i < history_count; i++)
        printf("%4d  %s", i + 1, history[i]);
}

static void execute_tokens(char **tokens, int n) {
    if (n == 0)
        return;

    if (strcmp(tokens[0], "cd") == 0) {
        if (n < 2 || chdir(tokens[1]) != 0)
            fprintf(stderr, "cd: could not change directory\n");
        return;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return;
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

        if (line[0] == '\n')
            continue;

        if (strcmp(line, "exit\n") == 0)
            break;

        if (strcmp(line, "history\n") == 0) {
            print_history();
            continue;
        }

        if (line[0] == '!') {
            int idx = atoi(line + 1);
            if (idx < 1 || idx > history_count) {
                fprintf(stderr, "history: no such entry\n");
                continue;
            }
            char entry_copy[1024];
            strncpy(entry_copy, history[idx - 1], sizeof(entry_copy) - 1);
            entry_copy[sizeof(entry_copy) - 1] = '\0';
            printf("%s", entry_copy);
            fflush(stdout);
            add_history(entry_copy);
            char exec_copy[1024];
            strncpy(exec_copy, entry_copy, sizeof(exec_copy) - 1);
            exec_copy[sizeof(exec_copy) - 1] = '\0';
            int n = tokenize(exec_copy, tokens);
            execute_tokens(tokens, n);
            continue;
        }

        add_history(line);

        char copy[1024];
        strncpy(copy, line, sizeof(copy) - 1);
        copy[sizeof(copy) - 1] = '\0';
        int n = tokenize(copy, tokens);
        execute_tokens(tokens, n);
    }

    for (int i = 0; i < history_count; i++)
        free(history[i]);

    return 0;
}
