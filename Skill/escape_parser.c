#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_LEN 256

/* Shell rules: a backslash makes the next character an ordinary character */
static int shell_words(const char *line) {
    int i = 0;
    char word[MAX_LEN];

    while (1) {
        while (isspace((unsigned char)line[i]))
            i++;
        if (line[i] == '\0')
            return 0;
        if (strchr("|<>&;", line[i]) != NULL) {
            printf("  operator: %c\n", line[i]);
            i++;
            continue;
        }

        int len = 0;
        while (line[i] != '\0' && !isspace((unsigned char)line[i]) && strchr("|<>&;", line[i]) == NULL) {
            if (line[i] == '\\') {
                if (line[i + 1] == '\0') {
                    printf("error: line ends with an escape character\n");
                    return -1;
                }
                i++;
            }
            if (len < MAX_LEN - 1)
                word[len++] = line[i];
            i++;
        }
        word[len] = '\0';
        printf("  word    : [%s]\n", word);
    }
}

static int hex_value(char c) {
    if (isdigit((unsigned char)c))
        return c - '0';
    return tolower((unsigned char)c) - 'a' + 10;
}

/* echo -e rules: \n, \t, \\, \a, \xHH and \0NNN become the bytes they name */
static void echo_e(const char *s) {
    printf("echo -e view:\n");
    for (int i = 0; s[i] != '\0'; i++) {
        if (s[i] != '\\' || s[i + 1] == '\0') {
            putchar(s[i]);
            continue;
        }
        i++;
        if (s[i] == 'n') {
            putchar('\n');
        } else if (s[i] == 't') {
            putchar('\t');
        } else if (s[i] == '\\') {
            putchar('\\');
        } else if (s[i] == 'a') {
            printf("<BEL>");
        } else if (s[i] == 'x' && isxdigit((unsigned char)s[i + 1])) {
            int value = 0;
            for (int k = 0; k < 2 && isxdigit((unsigned char)s[i + 1]); k++)
                value = value * 16 + hex_value(s[++i]);
            putchar(value);
        } else if (s[i] == '0') {
            int value = 0;
            for (int k = 0; k < 3 && s[i + 1] >= '0' && s[i + 1] <= '7'; k++)
                value = value * 8 + (s[++i] - '0');
            putchar(value);
        } else {
            putchar('\\');
            putchar(s[i]);
        }
    }
    putchar('\n');
}

int main(void) {
    char line[1024];

    while (1) {
        printf("escape> ");
        fflush(stdout);
        if (fgets(line, sizeof(line), stdin) == NULL) {
            printf("\n");
            break;
        }
        line[strcspn(line, "\n")] = '\0';
        if (strcmp(line, "exit") == 0)
            break;

        printf("shell view:\n");
        if (shell_words(line) == 0)
            echo_e(line);
    }
    return 0;
}
