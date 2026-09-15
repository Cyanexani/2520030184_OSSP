#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TOKENS 64
#define DELIMS " \t\r\n"

char **tokenize(char *line, int *count) {
    char **tokens = malloc(MAX_TOKENS * sizeof(char *));
    int n = 0;

    char *tok = strtok(line, DELIMS);
    while (tok != NULL && n < MAX_TOKENS - 1) {
        tokens[n++] = tok;
        tok = strtok(NULL, DELIMS);
    }
    tokens[n] = NULL;
    *count = n;
    return tokens;
}

int main(void) {
    char line[1024];

    printf("Enter a line to tokenize: ");
    if (fgets(line, sizeof(line), stdin) == NULL)
        return 0;

    int count;
    char **tokens = tokenize(line, &count);

    printf("Token count: %d\n", count);
    for (int i = 0; i < count; i++)
        printf("  [%d] \"%s\"\n", i, tokens[i]);

    free(tokens);
    return 0;
}
