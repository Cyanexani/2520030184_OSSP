#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

static const char *check(const char *path) {
    struct stat st;
    if (stat(path, &st) == -1)
        return "not found";
    if (!S_ISREG(st.st_mode))
        return "found but not a regular file";
    if (access(path, X_OK) != 0)
        return "found but not executable";
    return "FOUND";
}

static void resolve(const char *name) {
    char full[1024];
    char first[1024] = "";

    printf("%s:\n", name);
    if (strchr(name, '/') != NULL) {
        const char *result = check(name);
        printf("  %-36s %s\n", name, result);
        if (strcmp(result, "FOUND") == 0)
            snprintf(first, sizeof(first), "%s", name);
    } else {
        const char *path = getenv("PATH");
        char *copy = strdup(path != NULL ? path : "");
        char *rest = copy;
        char *dir;

        /* strsep keeps empty entries, and an empty entry means the current directory */
        while ((dir = strsep(&rest, ":")) != NULL) {
            if (*dir == '\0')
                dir = ".";
            snprintf(full, sizeof(full), "%s/%s", dir, name);
            const char *result = check(full);
            printf("  %-36s %s\n", full, result);
            if (first[0] == '\0' && strcmp(result, "FOUND") == 0)
                snprintf(first, sizeof(first), "%s", full);
        }
        free(copy);
    }

    if (first[0] != '\0')
        printf("resolved: %s\n\n", first);
    else
        printf("%s: not found in PATH\n\n", name);
}

int main(int argc, char *argv[]) {
    char line[1024];

    if (argc > 1) {
        for (int i = 1; i < argc; i++)
            resolve(argv[i]);
        return 0;
    }

    while (1) {
        printf("resolve> ");
        fflush(stdout);
        if (fgets(line, sizeof(line), stdin) == NULL) {
            printf("\n");
            break;
        }
        line[strcspn(line, " \t\n")] = '\0';
        if (strcmp(line, "exit") == 0)
            break;
        if (line[0] != '\0')
            resolve(line);
    }
    return 0;
}
