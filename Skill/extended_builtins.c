#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_ARGS 64
#define MAX_ALIASES 16
#define MAX_HISTORY 100
#define DELIMS " \t\r\n"

extern char **environ;

struct alias {
    char name[32];
    char value[256];
};

struct builtin {
    const char *name;
    int (*fn)(char **argv);
    const char *help;
};

static struct alias aliases[MAX_ALIASES];
static int alias_count = 0;
static char *history[MAX_HISTORY];
static int history_count = 0;
static int last_status = 0;
static int running = 1;
static int exit_code = 0;

static int tokenize(char *line, char **args) {
    int n = 0;
    char *tok = strtok(line, DELIMS);
    while (tok != NULL && n < MAX_ARGS - 1) {
        args[n++] = tok;
        tok = strtok(NULL, DELIMS);
    }
    args[n] = NULL;
    return n;
}

static const char *find_alias(const char *name) {
    for (int i = 0; i < alias_count; i++) {
        if (strcmp(aliases[i].name, name) == 0)
            return aliases[i].value;
    }
    return NULL;
}

static int sh_cd(char **argv) {
    const char *dir = argv[1] != NULL ? argv[1] : getenv("HOME");
    if (dir == NULL || chdir(dir) != 0) {
        perror("cd");
        return 1;
    }
    return 0;
}

static int sh_pwd(char **argv) {
    char cwd[1024];
    (void)argv;
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("pwd");
        return 1;
    }
    printf("%s\n", cwd);
    return 0;
}

static int sh_echo(char **argv) {
    for (int i = 1; argv[i] != NULL; i++) {
        if (i > 1)
            printf(" ");
        if (strcmp(argv[i], "$?") == 0) {
            printf("%d", last_status);
        } else if (argv[i][0] == '$') {
            const char *v = getenv(argv[i] + 1);
            printf("%s", v != NULL ? v : "");
        } else {
            printf("%s", argv[i]);
        }
    }
    printf("\n");
    return 0;
}

static int sh_export(char **argv) {
    for (int i = 1; argv[i] != NULL; i++) {
        char *eq = strchr(argv[i], '=');
        if (eq == NULL) {
            printf("usage: export NAME=value\n");
            return 1;
        }
        *eq = '\0';
        setenv(argv[i], eq + 1, 1);
    }
    return 0;
}

static int sh_unset(char **argv) {
    for (int i = 1; argv[i] != NULL; i++)
        unsetenv(argv[i]);
    return 0;
}

static int sh_env(char **argv) {
    (void)argv;
    for (char **e = environ; *e != NULL; e++)
        printf("%s\n", *e);
    return 0;
}

static int sh_alias(char **argv) {
    if (argv[1] == NULL) {
        for (int i = 0; i < alias_count; i++)
            printf("alias %s='%s'\n", aliases[i].name, aliases[i].value);
        return 0;
    }
    char *eq = strchr(argv[1], '=');
    if (eq == NULL) {
        const char *v = find_alias(argv[1]);
        if (v == NULL) {
            printf("alias: %s: not found\n", argv[1]);
            return 1;
        }
        printf("alias %s='%s'\n", argv[1], v);
        return 0;
    }

    /* everything after '=' is the alias text, including the other words */
    char value[256] = "";
    *eq = '\0';
    snprintf(value, sizeof(value), "%s", eq + 1);
    for (int i = 2; argv[i] != NULL; i++) {
        strncat(value, " ", sizeof(value) - strlen(value) - 1);
        strncat(value, argv[i], sizeof(value) - strlen(value) - 1);
    }
    int i = 0;
    while (i < alias_count && strcmp(aliases[i].name, argv[1]) != 0)
        i++;
    if (i == MAX_ALIASES)
        return 1;
    if (i == alias_count)
        alias_count++;
    snprintf(aliases[i].name, sizeof(aliases[i].name), "%s", argv[1]);
    snprintf(aliases[i].value, sizeof(aliases[i].value), "%s", value);
    return 0;
}

static int sh_unalias(char **argv) {
    for (int i = 0; i < alias_count; i++) {
        if (argv[1] != NULL && strcmp(aliases[i].name, argv[1]) == 0) {
            aliases[i] = aliases[--alias_count];
            return 0;
        }
    }
    printf("unalias: %s: not found\n", argv[1] != NULL ? argv[1] : "");
    return 1;
}

static int sh_history(char **argv) {
    (void)argv;
    for (int i = 0; i < history_count; i++)
        printf("%4d  %s\n", i + 1, history[i]);
    return 0;
}

static int sh_exit(char **argv) {
    exit_code = argv[1] != NULL ? atoi(argv[1]) : last_status;
    running = 0;
    return exit_code;
}

static int sh_type(char **argv);
static int sh_help(char **argv);

static struct builtin builtins[] = {
    {"cd", sh_cd, "change directory"},
    {"pwd", sh_pwd, "print working directory"},
    {"echo", sh_echo, "print arguments, $NAME and $? are expanded"},
    {"export", sh_export, "export NAME=value to the environment"},
    {"unset", sh_unset, "remove an environment variable"},
    {"env", sh_env, "list the environment"},
    {"alias", sh_alias, "define or list aliases"},
    {"unalias", sh_unalias, "remove an alias"},
    {"history", sh_history, "list previous commands"},
    {"type", sh_type, "say how a name would be run"},
    {"help", sh_help, "show this list"},
    {"exit", sh_exit, "leave the shell, optionally with a status"},
};

#define NUM_BUILTINS (int)(sizeof(builtins) / sizeof(builtins[0]))

static int is_builtin(const char *name) {
    for (int i = 0; i < NUM_BUILTINS; i++) {
        if (strcmp(builtins[i].name, name) == 0)
            return i;
    }
    return -1;
}

static int sh_type(char **argv) {
    if (argv[1] == NULL)
        return 1;
    const char *a = find_alias(argv[1]);
    if (a != NULL) {
        printf("%s is aliased to '%s'\n", argv[1], a);
        return 0;
    }
    if (is_builtin(argv[1]) >= 0) {
        printf("%s is a shell builtin\n", argv[1]);
        return 0;
    }

    const char *path = getenv("PATH");
    char *copy = strdup(path != NULL ? path : "");
    char full[1024];
    int found = 0;
    for (char *dir = strtok(copy, ":"); dir != NULL; dir = strtok(NULL, ":")) {
        snprintf(full, sizeof(full), "%s/%s", dir, argv[1]);
        if (access(full, X_OK) == 0) {
            printf("%s is %s\n", argv[1], full);
            found = 1;
            break;
        }
    }
    free(copy);
    if (!found)
        printf("%s: not found\n", argv[1]);
    return found ? 0 : 1;
}

static int sh_help(char **argv) {
    (void)argv;
    for (int i = 0; i < NUM_BUILTINS; i++)
        printf("  %-8s %s\n", builtins[i].name, builtins[i].help);
    return 0;
}

static int run_external(char **args) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    }
    if (pid == 0) {
        execvp(args[0], args);
        fprintf(stderr, "%s: command not found\n", args[0]);
        _exit(127);
    }
    int status;
    waitpid(pid, &status, 0);
    return WIFEXITED(status) ? WEXITSTATUS(status) : 128 + WTERMSIG(status);
}

int main(void) {
    char line[1024];
    char expanded[1024];
    char *args[MAX_ARGS];

    while (running) {
        printf("[%d] ext$ ", history_count + 1);
        fflush(stdout);
        if (fgets(line, sizeof(line), stdin) == NULL) {
            printf("\n");
            break;
        }
        line[strcspn(line, "\n")] = '\0';
        if (line[0] == '\0')
            continue;
        if (history_count < MAX_HISTORY)
            history[history_count++] = strdup(line);

        int n = tokenize(line, args);
        if (n == 0)
            continue;

        /* replace an alias in the first word and split the line again */
        const char *a = find_alias(args[0]);
        if (a != NULL) {
            snprintf(expanded, sizeof(expanded), "%s", a);
            for (int i = 1; i < n; i++) {
                strncat(expanded, " ", sizeof(expanded) - strlen(expanded) - 1);
                strncat(expanded, args[i], sizeof(expanded) - strlen(expanded) - 1);
            }
            n = tokenize(expanded, args);
            if (n == 0)
                continue;
        }

        int b = is_builtin(args[0]);
        if (b >= 0)
            last_status = builtins[b].fn(args);
        else
            last_status = run_external(args);
    }

    for (int i = 0; i < history_count; i++)
        free(history[i]);
    return exit_code;
}
