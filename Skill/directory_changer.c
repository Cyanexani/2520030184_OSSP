#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>

static void change_dir(const char *arg) {
    char target[1024];
    const char *home = getenv("HOME");
    int show = 0;

    if (home == NULL)
        home = "/";
    if (arg == NULL || strcmp(arg, "~") == 0) {
        snprintf(target, sizeof(target), "%s", home);
    } else if (strncmp(arg, "~/", 2) == 0) {
        snprintf(target, sizeof(target), "%s%s", home, arg + 1);
    } else if (strcmp(arg, "-") == 0) {
        const char *old = getenv("OLDPWD");
        if (old == NULL) {
            printf("cd: OLDPWD not set\n");
            return;
        }
        snprintf(target, sizeof(target), "%s", old);
        show = 1;
    } else {
        snprintf(target, sizeof(target), "%s", arg);
    }

    struct stat st;
    if (stat(target, &st) == -1) {
        printf("cd: %s: %s\n", target, strerror(errno));
        return;
    }
    if (!S_ISDIR(st.st_mode)) {
        printf("cd: %s: Not a directory\n", target);
        return;
    }

    char *before = getcwd(NULL, 0);
    if (chdir(target) == -1) {
        printf("cd: %s: %s\n", target, strerror(errno));
        free(before);
        return;
    }
    char *now = getcwd(NULL, 0);
    if (before != NULL)
        setenv("OLDPWD", before, 1);
    if (now != NULL)
        setenv("PWD", now, 1);
    if (show && now != NULL)
        printf("%s\n", now);
    free(before);
    free(now);
}

int main(void) {
    char line[1024];

    while (1) {
        char *cwd = getcwd(NULL, 0);
        printf("cd-demo:%s$ ", cwd != NULL ? cwd : "?");
        free(cwd);
        fflush(stdout);
        if (fgets(line, sizeof(line), stdin) == NULL) {
            printf("\n");
            break;
        }

        char *cmd = strtok(line, " \t\n");
        char *arg = strtok(NULL, " \t\n");
        if (cmd == NULL)
            continue;
        if (strcmp(cmd, "exit") == 0)
            break;

        if (strcmp(cmd, "cd") == 0) {
            change_dir(arg);
        } else if (strcmp(cmd, "pwd") == 0) {
            char *now = getcwd(NULL, 0);
            const char *pwd = getenv("PWD");
            printf("getcwd(): %s\n$PWD    : %s\n", now != NULL ? now : "?", pwd != NULL ? pwd : "(unset)");
            free(now);
        } else {
            printf("%s: only cd, pwd and exit are supported here\n", cmd);
        }
    }
    return 0;
}
