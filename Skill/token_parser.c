#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

enum token_type {
    WORD, PIPE, OR, AND, BACKGROUND, SEMICOLON, REDIRECT_IN, REDIRECT_OUT, APPEND
};

static const char *type_names[] = {
    "WORD", "PIPE", "OR", "AND", "BACKGROUND", "SEMICOLON", "REDIRECT_IN", "REDIRECT_OUT", "APPEND"
};

struct token {
    enum token_type type;
    char *text;
    int column;
};

static int tokenize(const char *line, struct token **out) {
    int cap = 8, n = 0, i = 0;
    struct token *tokens = malloc(cap * sizeof(struct token));

    while (line[i] != '\0') {
        if (isspace((unsigned char)line[i])) {
            i++;
            continue;
        }

        int start = i;
        enum token_type type;
        if (strncmp(line + i, "||", 2) == 0) {
            type = OR;
            i += 2;
        } else if (strncmp(line + i, "&&", 2) == 0) {
            type = AND;
            i += 2;
        } else if (strncmp(line + i, ">>", 2) == 0) {
            type = APPEND;
            i += 2;
        } else if (line[i] == '|') {
            type = PIPE;
            i++;
        } else if (line[i] == '&') {
            type = BACKGROUND;
            i++;
        } else if (line[i] == ';') {
            type = SEMICOLON;
            i++;
        } else if (line[i] == '<') {
            type = REDIRECT_IN;
            i++;
        } else if (line[i] == '>') {
            type = REDIRECT_OUT;
            i++;
        } else {
            type = WORD;
            while (line[i] != '\0' && !isspace((unsigned char)line[i]) && strchr("|&;<>", line[i]) == NULL)
                i++;
        }

        if (n == cap) {
            cap *= 2;
            tokens = realloc(tokens, cap * sizeof(struct token));
        }
        tokens[n].type = type;
        tokens[n].text = strndup(line + start, i - start);
        tokens[n].column = start + 1;
        n++;
    }

    *out = tokens;
    return n;
}

int main(void) {
    char line[1024];
    struct token *tokens;

    while (1) {
        printf("tokens> ");
        fflush(stdout);
        if (fgets(line, sizeof(line), stdin) == NULL) {
            printf("\n");
            break;
        }
        line[strcspn(line, "\n")] = '\0';
        if (strcmp(line, "exit") == 0)
            break;

        int n = tokenize(line, &tokens);
        printf("%-3s %-13s %-6s %s\n", "#", "TYPE", "COLUMN", "TEXT");
        for (int i = 0; i < n; i++) {
            printf("%-3d %-13s %-6d %s\n", i, type_names[tokens[i].type], tokens[i].column, tokens[i].text);
            free(tokens[i].text);
        }
        printf("%d tokens\n", n);
        free(tokens);
    }
    return 0;
}
