#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_ARGS 16
#define MAX_CMDS 8
#define MAX_TOKENS 64

struct stage {
    char *argv[MAX_ARGS];
    int argc;
};

struct job {
    struct stage stages[MAX_CMDS];
    int count;
    char *in_file;
    char *out_file;
    int append;
};

static char token_buf[MAX_TOKENS][256];

static int open_fds(void) {
    int n = 0;
    DIR *d = opendir("/proc/self/fd");
    if (d == NULL)
        return -1;
    while (readdir(d) != NULL)
        n++;
    closedir(d);
    /* leave out ".", ".." and the descriptor opendir itself used */
    return n - 3;
}

static int tokenize(const char *line, char **tokens) {
    int n = 0, i = 0;
    while (line[i] != '\0' && n < MAX_TOKENS - 1) {
        if (isspace((unsigned char)line[i])) {
            i++;
            continue;
        }
        int len = 0;
        if (strncmp(line + i, ">>", 2) == 0) {
            len = 2;
        } else if (strchr("|<>", line[i]) != NULL) {
            len = 1;
        } else {
            while (line[i + len] != '\0' && !isspace((unsigned char)line[i + len]) &&
                   strchr("|<>", line[i + len]) == NULL)
                len++;
        }
        snprintf(token_buf[n], sizeof(token_buf[n]), "%.*s", len, line + i);
        tokens[n] = token_buf[n];
        n++;
        i += len;
    }
    tokens[n] = NULL;
    return n;
}

static int parse(char **tokens, int n, struct job *job) {
    memset(job, 0, sizeof(*job));
    job->count = 1;
    for (int i = 0; i < n; i++) {
        struct stage *s = &job->stages[job->count - 1];
        if (strcmp(tokens[i], "|") == 0) {
            if (s->argc == 0 || job->count == MAX_CMDS) {
                printf("syntax error: empty command in pipeline\n");
                return -1;
            }
            job->count++;
        } else if (strcmp(tokens[i], "<") == 0 || strcmp(tokens[i], ">") == 0 ||
                   strcmp(tokens[i], ">>") == 0) {
            if (i + 1 == n || strchr("|<>", tokens[i + 1][0]) != NULL) {
                printf("syntax error: missing file name after `%s'\n", tokens[i]);
                return -1;
            }
            if (tokens[i][0] == '<') {
                job->in_file = tokens[++i];
            } else {
                job->append = tokens[i][1] == '>';
                job->out_file = tokens[++i];
            }
        } else if (s->argc < MAX_ARGS - 1) {
            s->argv[s->argc++] = tokens[i];
        }
    }
    if (job->stages[job->count - 1].argc == 0) {
        printf("syntax error: empty command in pipeline\n");
        return -1;
    }
    return 0;
}

static void print_plan(struct job *job) {
    for (int i = 0; i < job->count; i++) {
        char in[300] = "terminal", out[300] = "terminal";
        if (i == 0 && job->in_file != NULL)
            snprintf(in, sizeof(in), "file %s", job->in_file);
        else if (i > 0)
            snprintf(in, sizeof(in), "pipe %d", i - 1);
        if (i == job->count - 1 && job->out_file != NULL)
            snprintf(out, sizeof(out), "file %s (%s)", job->out_file, job->append ? "append" : "truncate");
        else if (i < job->count - 1)
            snprintf(out, sizeof(out), "pipe %d", i);

        printf("  stage %d: %s", i, job->stages[i].argv[0]);
        for (int j = 1; j < job->stages[i].argc; j++)
            printf(" %s", job->stages[i].argv[j]);
        printf("   stdin: %s   stdout: %s\n", in, out);
    }
}

static void child(struct job *job, int i, int pipes[][2]) {
    if (i == 0 && job->in_file != NULL) {
        int fd = open(job->in_file, O_RDONLY);
        if (fd == -1) {
            perror(job->in_file);
            _exit(1);
        }
        dup2(fd, STDIN_FILENO);
        close(fd);
    }
    if (i == job->count - 1 && job->out_file != NULL) {
        int flags = O_WRONLY | O_CREAT | (job->append ? O_APPEND : O_TRUNC);
        int fd = open(job->out_file, flags, 0644);
        if (fd == -1) {
            perror(job->out_file);
            _exit(1);
        }
        dup2(fd, STDOUT_FILENO);
        close(fd);
    }
    if (i > 0)
        dup2(pipes[i - 1][0], STDIN_FILENO);
    if (i < job->count - 1)
        dup2(pipes[i][1], STDOUT_FILENO);
    for (int j = 0; j < job->count - 1; j++) {
        close(pipes[j][0]);
        close(pipes[j][1]);
    }
    execvp(job->stages[i].argv[0], job->stages[i].argv);
    perror(job->stages[i].argv[0]);
    _exit(127);
}

static void run(struct job *job) {
    int pipes[MAX_CMDS - 1][2];
    pid_t pids[MAX_CMDS];
    int started = 0;

    for (int i = 0; i < job->count - 1; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            return;
        }
    }
    for (int i = 0; i < job->count; i++) {
        pids[i] = fork();
        if (pids[i] < 0) {
            perror("fork");
            break;
        }
        if (pids[i] == 0)
            child(job, i, pipes);
        started++;
    }
    for (int j = 0; j < job->count - 1; j++) {
        close(pipes[j][0]);
        close(pipes[j][1]);
    }
    int status[MAX_CMDS];
    for (int i = 0; i < started; i++)
        waitpid(pids[i], &status[i], 0);
    for (int i = 0; i < started; i++)
        printf("  stage %d (pid %d) exited with status %d\n", i, pids[i], WEXITSTATUS(status[i]));
}

int main(void) {
    char line[1024];
    char *tokens[MAX_TOKENS];
    struct job job;

    while (1) {
        printf("redirect> ");
        fflush(stdout);
        if (fgets(line, sizeof(line), stdin) == NULL) {
            printf("\n");
            break;
        }
        line[strcspn(line, "\n")] = '\0';
        if (strcmp(line, "exit") == 0)
            break;

        int n = tokenize(line, tokens);
        if (n == 0 || parse(tokens, n, &job) < 0)
            continue;

        int before = open_fds();
        print_plan(&job);
        fflush(stdout);
        run(&job);
        printf("  open descriptors in this process: %d before, %d after\n", before, open_fds());
    }
    return 0;
}
