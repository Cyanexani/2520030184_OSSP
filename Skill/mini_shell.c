#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_TOKENS 128
#define MAX_ARGS 32
#define MAX_STAGES 8
#define MAX_JOBS 16
#define MAX_HISTORY 100

enum { T_WORD, T_PIPE, T_IN, T_OUT, T_APPEND, T_BG, T_SEMI };

struct token {
    int type;
    char *text;
};

struct stage {
    char *argv[MAX_ARGS];
    int argc;
};

struct command {
    struct stage stages[MAX_STAGES];
    int count;
    char *in_file;
    char *out_file;
    int append;
};

struct job {
    int used;
    pid_t pids[MAX_STAGES];
    int done[MAX_STAGES];
    int count;
    char text[256];
};

struct buffer {
    char *data;
    size_t len, cap;
};

static struct job jobs[MAX_JOBS];
static char *history[MAX_HISTORY];
static int history_count = 0;
static int last_status = 0;
static int running = 1;

static void buf_add(struct buffer *b, char c) {
    if (b->len + 2 > b->cap) {
        b->cap = b->cap ? b->cap * 2 : 32;
        b->data = realloc(b->data, b->cap);
    }
    b->data[b->len++] = c;
    b->data[b->len] = '\0';
}

static void buf_add_str(struct buffer *b, const char *s) {
    while (*s != '\0')
        buf_add(b, *s++);
}

/* line[*i] is '$': append the value of $?, $NAME or ${NAME} */
static void expand_var(const char *line, int *i, struct buffer *w) {
    char name[64];
    int j = 0, k = *i + 1;

    if (line[k] == '?') {
        char num[16];
        snprintf(num, sizeof(num), "%d", last_status);
        buf_add_str(w, num);
        *i = k + 1;
        return;
    }
    int braces = line[k] == '{';
    if (braces)
        k++;
    while ((isalnum((unsigned char)line[k]) || line[k] == '_') && j < 63)
        name[j++] = line[k++];
    name[j] = '\0';
    if (braces && line[k] == '}')
        k++;
    if (j == 0) {
        buf_add(w, '$');
        *i = *i + 1;
        return;
    }
    const char *value = getenv(name);
    if (value != NULL)
        buf_add_str(w, value);
    *i = k;
}

/*
 * Reads one command starting at line[*pos], stopping after ; or & so that
 * $? and $VAR are expanded only when the previous command has finished.
 * *sep is set to T_SEMI, T_BG or -1 at the end of the line.
 */
static int tokenize(const char *line, int *pos, struct token *t, int *sep) {
    int n = 0, i = *pos;

    *sep = -1;
    while (line[i] != '\0' && n < MAX_TOKENS) {
        char c = line[i];
        if (isspace((unsigned char)c)) {
            i++;
            continue;
        }
        if (c == ';' || c == '&') {
            *sep = c == ';' ? T_SEMI : T_BG;
            i++;
            break;
        }
        if (strchr("|<>", c) != NULL) {
            t[n].text = NULL;
            if (c == '>' && line[i + 1] == '>') {
                t[n].type = T_APPEND;
                i++;
            } else {
                t[n].type = c == '|' ? T_PIPE : c == '<' ? T_IN : T_OUT;
            }
            n++;
            i++;
            continue;
        }

        struct buffer w = {NULL, 0, 0};
        int quoted = 0;
        while (line[i] != '\0' && !isspace((unsigned char)line[i]) && strchr("|<>&;", line[i]) == NULL) {
            if (line[i] == '\\') {
                if (line[i + 1] != '\0')
                    buf_add(&w, line[i + 1]);
                i += line[i + 1] != '\0' ? 2 : 1;
            } else if (line[i] == '\'') {
                for (i++; line[i] != '\0' && line[i] != '\''; i++)
                    buf_add(&w, line[i]);
                if (line[i] == '\0')
                    goto unterminated;
                i++;
                quoted = 1;
            } else if (line[i] == '"') {
                i++;
                while (line[i] != '\0' && line[i] != '"') {
                    if (line[i] == '\\' && line[i + 1] != '\0' && strchr("\"\\$", line[i + 1]) != NULL) {
                        buf_add(&w, line[i + 1]);
                        i += 2;
                    } else if (line[i] == '$') {
                        expand_var(line, &i, &w);
                    } else {
                        buf_add(&w, line[i++]);
                    }
                }
                if (line[i] == '\0')
                    goto unterminated;
                i++;
                quoted = 1;
            } else if (line[i] == '$') {
                expand_var(line, &i, &w);
            } else {
                buf_add(&w, line[i++]);
            }
        }
        if (w.len > 0 || quoted) {
            t[n].type = T_WORD;
            t[n].text = w.data != NULL ? w.data : strdup("");
            n++;
        }
        continue;

unterminated:
        free(w.data);
        for (int k = 0; k < n; k++)
            free(t[k].text);
        return -1;
    }
    *pos = i;
    return n;
}

static int parse(struct token *t, int end, struct command *cmd) {
    memset(cmd, 0, sizeof(*cmd));
    cmd->count = 1;
    for (int k = 0; k < end; k++) {
        struct stage *s = &cmd->stages[cmd->count - 1];
        if (t[k].type == T_WORD) {
            if (s->argc < MAX_ARGS - 1)
                s->argv[s->argc++] = t[k].text;
        } else if (t[k].type == T_PIPE) {
            if (s->argc == 0 || cmd->count == MAX_STAGES) {
                fprintf(stderr, "syntax error near `|'\n");
                return -1;
            }
            cmd->count++;
        } else {
            if (k + 1 >= end || t[k + 1].type != T_WORD) {
                fprintf(stderr, "syntax error: missing file name for redirection\n");
                return -1;
            }
            if (t[k].type == T_IN) {
                cmd->in_file = t[k + 1].text;
            } else {
                cmd->out_file = t[k + 1].text;
                cmd->append = t[k].type == T_APPEND;
            }
            k++;
        }
    }
    if (cmd->stages[cmd->count - 1].argc == 0) {
        fprintf(stderr, "syntax error: missing command\n");
        return -1;
    }
    return 0;
}

static int builtin(char **argv) {
    if (strcmp(argv[0], "cd") == 0) {
        const char *dir = argv[1];
        if (dir == NULL)
            dir = getenv("HOME");
        else if (strcmp(dir, "-") == 0)
            dir = getenv("OLDPWD");
        char *old = getcwd(NULL, 0);
        if (dir == NULL || chdir(dir) != 0) {
            perror("cd");
            free(old);
            return 1;
        }
        if (old != NULL)
            setenv("OLDPWD", old, 1);
        free(old);
        char *now = getcwd(NULL, 0);
        if (now != NULL)
            setenv("PWD", now, 1);
        if (argv[1] != NULL && strcmp(argv[1], "-") == 0 && now != NULL)
            printf("%s\n", now);
        free(now);
        return 0;
    }
    if (strcmp(argv[0], "pwd") == 0) {
        char *cwd = getcwd(NULL, 0);
        if (cwd == NULL) {
            perror("pwd");
            return 1;
        }
        printf("%s\n", cwd);
        free(cwd);
        return 0;
    }
    if (strcmp(argv[0], "echo") == 0) {
        for (int i = 1; argv[i] != NULL; i++)
            printf("%s%s", i > 1 ? " " : "", argv[i]);
        printf("\n");
        return 0;
    }
    if (strcmp(argv[0], "export") == 0) {
        for (int i = 1; argv[i] != NULL; i++) {
            char *eq = strchr(argv[i], '=');
            if (eq == NULL)
                continue;
            *eq = '\0';
            setenv(argv[i], eq + 1, 1);
            *eq = '=';
        }
        return 0;
    }
    if (strcmp(argv[0], "unset") == 0) {
        for (int i = 1; argv[i] != NULL; i++)
            unsetenv(argv[i]);
        return 0;
    }
    if (strcmp(argv[0], "history") == 0) {
        for (int i = 0; i < history_count; i++)
            printf("%4d  %s\n", i + 1, history[i]);
        return 0;
    }
    if (strcmp(argv[0], "help") == 0) {
        printf("built-ins: cd [dir|-], pwd, echo, export NAME=value, unset NAME, history, help, exit [n]\n");
        printf("also: cmd | cmd, < file, > file, >> file, cmd ; cmd, cmd &, 'quotes', \"quotes\", $VAR, $?\n");
        return 0;
    }
    if (strcmp(argv[0], "exit") == 0) {
        running = 0;
        return argv[1] != NULL ? atoi(argv[1]) : last_status;
    }
    return -1;
}

static void describe(struct command *cmd, char *out, size_t size) {
    out[0] = '\0';
    for (int i = 0; i < cmd->count; i++) {
        for (int j = 0; j < cmd->stages[i].argc; j++) {
            strncat(out, cmd->stages[i].argv[j], size - strlen(out) - 1);
            strncat(out, " ", size - strlen(out) - 1);
        }
        if (i < cmd->count - 1)
            strncat(out, "| ", size - strlen(out) - 1);
    }
    out[strlen(out) - 1] = '\0';
}

static void start_child(struct command *cmd, int i, int pipes[][2], int background) {
    signal(SIGINT, SIG_DFL);
    /* a background job gets its own process group so Ctrl-C does not reach it */
    if (background)
        setpgid(0, 0);
    if (i == 0 && cmd->in_file != NULL) {
        int fd = open(cmd->in_file, O_RDONLY);
        if (fd == -1) {
            perror(cmd->in_file);
            _exit(1);
        }
        dup2(fd, STDIN_FILENO);
        close(fd);
    }
    if (i == cmd->count - 1 && cmd->out_file != NULL) {
        int fd = open(cmd->out_file, O_WRONLY | O_CREAT | (cmd->append ? O_APPEND : O_TRUNC), 0644);
        if (fd == -1) {
            perror(cmd->out_file);
            _exit(1);
        }
        dup2(fd, STDOUT_FILENO);
        close(fd);
    }
    if (i > 0)
        dup2(pipes[i - 1][0], STDIN_FILENO);
    if (i < cmd->count - 1)
        dup2(pipes[i][1], STDOUT_FILENO);
    for (int j = 0; j < cmd->count - 1; j++) {
        close(pipes[j][0]);
        close(pipes[j][1]);
    }
    execvp(cmd->stages[i].argv[0], cmd->stages[i].argv);
    fprintf(stderr, "%s: command not found\n", cmd->stages[i].argv[0]);
    _exit(127);
}

static void run_command(struct command *cmd, int background) {
    int pipes[MAX_STAGES - 1][2];
    pid_t pids[MAX_STAGES];
    int started = 0;

    if (!background && cmd->count == 1 && cmd->in_file == NULL && cmd->out_file == NULL) {
        int status = builtin(cmd->stages[0].argv);
        if (status >= 0) {
            last_status = status;
            return;
        }
    }

    for (int i = 0; i < cmd->count - 1; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            return;
        }
    }
    fflush(stdout);
    for (int i = 0; i < cmd->count; i++) {
        pids[i] = fork();
        if (pids[i] < 0) {
            perror("fork");
            break;
        }
        if (pids[i] == 0)
            start_child(cmd, i, pipes, background);
        started++;
    }
    for (int j = 0; j < cmd->count - 1; j++) {
        close(pipes[j][0]);
        close(pipes[j][1]);
    }

    if (background) {
        int id = 0;
        while (id < MAX_JOBS && jobs[id].used)
            id++;
        if (id == MAX_JOBS) {
            fprintf(stderr, "too many background jobs\n");
            return;
        }
        struct job *job = &jobs[id];
        job->used = 1;
        job->count = started;
        for (int i = 0; i < started; i++) {
            job->pids[i] = pids[i];
            job->done[i] = 0;
        }
        describe(cmd, job->text, sizeof(job->text));
        printf("[%d] %d\n", id + 1, pids[started - 1]);
        last_status = 0;
        return;
    }

    for (int i = 0; i < started; i++) {
        int status;
        waitpid(pids[i], &status, 0);
        if (i == started - 1)
            last_status = WIFEXITED(status) ? WEXITSTATUS(status) : 128 + WTERMSIG(status);
    }
}

static void reap_jobs(void) {
    for (int id = 0; id < MAX_JOBS; id++) {
        if (!jobs[id].used)
            continue;
        int finished = 1;
        for (int i = 0; i < jobs[id].count; i++) {
            if (jobs[id].done[i])
                continue;
            pid_t r = waitpid(jobs[id].pids[i], NULL, WNOHANG);
            if (r == 0)
                finished = 0;
            else
                jobs[id].done[i] = 1;
        }
        if (finished) {
            printf("[%d] Done    %s\n", id + 1, jobs[id].text);
            jobs[id].used = 0;
        }
    }
}

static void print_prompt(void) {
    const char *user = getenv("USER");
    const char *home = getenv("HOME");
    char *cwd = getcwd(NULL, 0);
    size_t hl = home != NULL ? strlen(home) : 0;

    printf("%s@ossp:", user != NULL ? user : "user");
    if (cwd != NULL && hl > 0 && strncmp(cwd, home, hl) == 0 && (cwd[hl] == '/' || cwd[hl] == '\0'))
        printf("~%s$ ", cwd + hl);
    else
        printf("%s$ ", cwd != NULL ? cwd : "?");
    free(cwd);
    fflush(stdout);
}

int main(void) {
    char line[1024];
    struct token tokens[MAX_TOKENS];
    struct command cmd;

    signal(SIGINT, SIG_IGN);

    while (running) {
        reap_jobs();
        print_prompt();
        if (fgets(line, sizeof(line), stdin) == NULL) {
            printf("\n");
            break;
        }
        line[strcspn(line, "\n")] = '\0';
        if (line[0] == '\0')
            continue;
        if (history_count < MAX_HISTORY)
            history[history_count++] = strdup(line);

        int pos = 0, sep;
        while (running && line[pos] != '\0') {
            int n = tokenize(line, &pos, tokens, &sep);
            if (n < 0) {
                fprintf(stderr, "syntax error: unterminated quote\n");
                last_status = 2;
                break;
            }
            if (n > 0) {
                if (parse(tokens, n, &cmd) == 0)
                    run_command(&cmd, sep == T_BG);
                else
                    last_status = 2;
            }
            for (int k = 0; k < n; k++)
                free(tokens[k].text);
        }
    }

    for (int i = 0; i < history_count; i++)
        free(history[i]);
    return last_status;
}
