#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_WORDS 32
#define MAX_LEN 256

static void append(char *word, int *len, const char *s) {
    while (*s != '\0' && *len < MAX_LEN - 1)
        word[(*len)++] = *s++;
}

/* Returns the number of words, or -1 when a single quote is not closed */
static int split_words(const char *line, char words[][MAX_LEN]) {
    int n = 0, i = 0;

    while (n < MAX_WORDS) {
        while (isspace((unsigned char)line[i]))
            i++;
        if (line[i] == '\0')
            break;

        char *w = words[n];
        int len = 0;
        while (line[i] != '\0' && !isspace((unsigned char)line[i])) {
            if (line[i] == '\'') {
                i++;
                while (line[i] != '\0' && line[i] != '\'') {
                    if (len < MAX_LEN - 1)
                        w[len++] = line[i];
                    i++;
                }
                if (line[i] == '\0')
                    return -1;
                i++;
            } else if (line[i] == '$' && (isalpha((unsigned char)line[i + 1]) || line[i + 1] == '_')) {
                char name[64];
                int j = 0;
                i++;
                while ((isalnum((unsigned char)line[i]) || line[i] == '_') && j < 63)
                    name[j++] = line[i++];
                name[j] = '\0';
                const char *value = getenv(name);
                if (value != NULL)
                    append(w, &len, value);
            } else {
                if (len < MAX_LEN - 1)
                    w[len++] = line[i];
                i++;
            }
        }
        w[len] = '\0';
        n++;
    }
    return n;
}

int main(void) {
    char line[1024];
    char words[MAX_WORDS][MAX_LEN];

    while (1) {
        printf("squote> ");
        fflush(stdout);
        if (fgets(line, sizeof(line), stdin) == NULL) {
            printf("\n");
            break;
        }
        line[strcspn(line, "\n")] = '\0';
        if (strcmp(line, "exit") == 0)
            break;

        int n = split_words(line, words);
        if (n < 0) {
            printf("error: unterminated single quote\n");
            continue;
        }
        for (int i = 0; i < n; i++)
            printf("  word %d: [%s]\n", i, words[i]);
    }
    return 0;
}
