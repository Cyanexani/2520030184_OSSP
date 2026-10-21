#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_ARGS 16
#define MAX_CMDS 8

struct command {
    char *argv[MAX_ARGS];
    int argc;
};

struct pipeline {
    struct command cmds[MAX_CMDS];
    int count;
};

static int parse_pipeline(char *line, struct pipeline *p) {
    char *rest = line;
    char *stage;

    p->count = 0;
    while ((stage = strsep(&rest, "|")) != NULL) {
        if (p->count == MAX_CMDS) {
            printf("too many commands, the limit is %d\n", MAX_CMDS);
            return -1;
        }
        struct command *c = &p->cmds[p->count];
        char *save;
        c->argc = 0;
        for (char *tok = strtok_r(stage, " \t\n", &save); tok != NULL && c->argc < MAX_ARGS - 1;
             tok = strtok_r(NULL, " \t\n", &save))
            c->argv[c->argc++] = tok;
        c->argv[c->argc] = NULL;
        if (c->argc == 0) {
            printf("syntax error: empty command in pipeline\n");
            return -1;
        }
        p->count++;
    }
    return 0;
}

static void print_pipeline(struct pipeline *p) {
    printf("pipeline with %d stage%s, %d pipe%s\n", p->count, p->count == 1 ? "" : "s",
           p->count - 1, p->count == 2 ? "" : "s");
    for (int i = 0; i < p->count; i++) {
        char in[16] = "terminal", out[16] = "terminal";
        if (i > 0)
            snprintf(in, sizeof(in), "pipe %d", i - 1);
        if (i < p->count - 1)
            snprintf(out, sizeof(out), "pipe %d", i);

        printf("  stage %d: argv = [", i);
        for (int j = 0; j < p->cmds[i].argc; j++)
            printf("%s%s", j ? ", " : "", p->cmds[i].argv[j]);
        printf("]  stdin: %s  stdout: %s\n", in, out);
    }
}

static void run_pipeline(struct pipeline *p) {
    int pipes[MAX_CMDS - 1][2];
    pid_t pids[MAX_CMDS];
    int started = 0;

    for (int i = 0; i < p->count - 1; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            return;
        }
    }

    for (int i = 0; i < p->count; i++) {
        pids[i] = fork();
        if (pids[i] < 0) {
            perror("fork");
            break;
        }
        if (pids[i] == 0) {
            if (i > 0)
                dup2(pipes[i - 1][0], STDIN_FILENO);
            if (i < p->count - 1)
                dup2(pipes[i][1], STDOUT_FILENO);
            for (int j = 0; j < p->count - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            execvp(p->cmds[i].argv[0], p->cmds[i].argv);
            perror(p->cmds[i].argv[0]);
            _exit(127);
        }
        started++;
    }

    /* the parent must close its copies or the last reader never sees end of file */
    for (int j = 0; j < p->count - 1; j++) {
        close(pipes[j][0]);
        close(pipes[j][1]);
    }

    int status[MAX_CMDS];
    for (int i = 0; i < started; i++)
        waitpid(pids[i], &status[i], 0);
    for (int i = 0; i < started; i++)
        printf("  stage %d (%s, pid %d) exited with status %d\n", i, p->cmds[i].argv[0], pids[i],
               WEXITSTATUS(status[i]));
}

int main(void) {
    char line[1024];
    struct pipeline p;

    while (1) {
        printf("pipeline> ");
        fflush(stdout);
        if (fgets(line, sizeof(line), stdin) == NULL) {
            printf("\n");
            break;
        }
        line[strcspn(line, "\n")] = '\0';
        if (strcmp(line, "exit") == 0)
            break;
        if (strspn(line, " \t") == strlen(line))
            continue;

        if (parse_pipeline(line, &p) < 0)
            continue;
        print_pipeline(&p);
        printf("output:\n");
        fflush(stdout);
        run_pipeline(&p);
    }
    return 0;
}
