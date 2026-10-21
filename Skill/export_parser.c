#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_ARGS 64
#define MAX_VARS 32

extern char **environ;

struct var {
    char name[64];
    char value[256];
};

static struct var shell_vars[MAX_VARS];
static int nvars = 0;

/* Splits on spaces but keeps quoted text together and drops the quotes */
static int tokenize(char *line, char **args) {
    int n = 0;
    char *src = line, *dst = line;

    while (*src != '\0' && n < MAX_ARGS - 1) {
        while (isspace((unsigned char)*src))
            src++;
        if (*src == '\0')
            break;
        args[n++] = dst;
        char quote = 0;
        while (*src != '\0' && (quote || !isspace((unsigned char)*src))) {
            if (quote == 0 && (*src == '"' || *src == '\'')) {
                quote = *src++;
            } else if (quote != 0 && *src == quote) {
                quote = 0;
                src++;
            } else {
                *dst++ = *src++;
            }
        }
        if (*src != '\0')
            src++;
        *dst++ = '\0';
    }
    args[n] = NULL;
    return n;
}

static int valid_name(const char *s, size_t len) {
    if (len == 0 || (!isalpha((unsigned char)s[0]) && s[0] != '_'))
        return 0;
    for (size_t i = 1; i < len; i++) {
        if (!isalnum((unsigned char)s[i]) && s[i] != '_')
            return 0;
    }
    return 1;
}

static int find_var(const char *name) {
    for (int i = 0; i < nvars; i++) {
        if (strcmp(shell_vars[i].name, name) == 0)
            return i;
    }
    return -1;
}

static void remove_var(const char *name) {
    int i = find_var(name);
    if (i >= 0)
        shell_vars[i] = shell_vars[--nvars];
}

static void set_shell_var(const char *name, const char *value) {
    int i = find_var(name);
    if (i < 0) {
        if (nvars == MAX_VARS)
            return;
        i = nvars++;
    }
    snprintf(shell_vars[i].name, sizeof(shell_vars[i].name), "%s", name);
    snprintf(shell_vars[i].value, sizeof(shell_vars[i].value), "%s", value);
}

static int compare(const void *a, const void *b) {
    return strcmp(*(char *const *)a, *(char *const *)b);
}

static void list_exported(void) {
    int n = 0;
    for (char **e = environ; *e != NULL; e++)
        n++;
    char **sorted = malloc(n * sizeof(char *));
    for (int i = 0; i < n; i++)
        sorted[i] = environ[i];
    qsort(sorted, n, sizeof(char *), compare);
    for (int i = 0; i < n; i++) {
        char *eq = strchr(sorted[i], '=');
        printf("declare -x %.*s=\"%s\"\n", (int)(eq - sorted[i]), sorted[i], eq + 1);
    }
    free(sorted);
}

static void do_export(char **args) {
    if (args[1] == NULL || strcmp(args[1], "-p") == 0) {
        list_exported();
        return;
    }
    for (int i = 1; args[i] != NULL; i++) {
        char *eq = strchr(args[i], '=');
        size_t len = eq != NULL ? (size_t)(eq - args[i]) : strlen(args[i]);
        if (!valid_name(args[i], len)) {
            printf("export: `%s': not a valid identifier\n", args[i]);
            continue;
        }
        if (eq != NULL) {
            *eq = '\0';
            setenv(args[i], eq + 1, 1);
        } else {
            int v = find_var(args[i]);
            setenv(args[i], v >= 0 ? shell_vars[v].value : "", 1);
        }
        remove_var(args[i]);
    }
}

static void run_external(char **args) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return;
    }
    if (pid == 0) {
        execvp(args[0], args);
        fprintf(stderr, "%s: command not found\n", args[0]);
        _exit(127);
    }
    waitpid(pid, NULL, 0);
}

int main(void) {
    char line[1024];
    char *args[MAX_ARGS];

    while (1) {
        printf("export> ");
        fflush(stdout);
        if (fgets(line, sizeof(line), stdin) == NULL) {
            printf("\n");
            break;
        }
        line[strcspn(line, "\n")] = '\0';

        int n = tokenize(line, args);
        if (n == 0)
            continue;
        if (strcmp(args[0], "exit") == 0)
            break;

        char *eq = strchr(args[0], '=');
        if (strcmp(args[0], "export") == 0) {
            do_export(args);
        } else if (strcmp(args[0], "unset") == 0) {
            for (int i = 1; args[i] != NULL; i++) {
                remove_var(args[i]);
                unsetenv(args[i]);
            }
        } else if (strcmp(args[0], "set") == 0) {
            for (int i = 0; i < nvars; i++)
                printf("%s=%s (shell only)\n", shell_vars[i].name, shell_vars[i].value);
        } else if (eq != NULL && n == 1 && valid_name(args[0], eq - args[0])) {
            *eq = '\0';
            if (getenv(args[0]) != NULL)
                setenv(args[0], eq + 1, 1);
            else
                set_shell_var(args[0], eq + 1);
        } else {
            run_external(args);
        }
    }
    return 0;
}
