#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_TOKENS 64
#define MAX_ARGS 16
#define MAX_REDIRS 4
#define MAX_CMDS 8
#define MAX_PIPELINES 8

enum token_type { WORD, PIPE, OR, AND, BACKGROUND, SEMICOLON, REDIRECT_IN, REDIRECT_OUT, APPEND, END };

struct token {
    enum token_type type;
    char text[64];
    int column;
};

struct command {
    char *argv[MAX_ARGS];
    int argc;
    struct token *redirs[MAX_REDIRS];
    char *files[MAX_REDIRS];
    int nredirs;
};

struct pipeline {
    struct command cmds[MAX_CMDS];
    int count;
};

struct list {
    struct pipeline pipelines[MAX_PIPELINES];
    const char *ops[MAX_PIPELINES];
    int count;
};

static struct token tokens[MAX_TOKENS];
static int ntokens, pos;
static char error[128];

static int tokenize(const char *line) {
    int i = 0;
    ntokens = 0;
    while (line[i] != '\0' && ntokens < MAX_TOKENS - 1) {
        if (isspace((unsigned char)line[i])) {
            i++;
            continue;
        }
        struct token *t = &tokens[ntokens++];
        int start = i, len = 1;
        if (strncmp(line + i, "||", 2) == 0) {
            t->type = OR;
            len = 2;
        } else if (strncmp(line + i, "&&", 2) == 0) {
            t->type = AND;
            len = 2;
        } else if (strncmp(line + i, ">>", 2) == 0) {
            t->type = APPEND;
            len = 2;
        } else if (line[i] == '|') {
            t->type = PIPE;
        } else if (line[i] == '&') {
            t->type = BACKGROUND;
        } else if (line[i] == ';') {
            t->type = SEMICOLON;
        } else if (line[i] == '<') {
            t->type = REDIRECT_IN;
        } else if (line[i] == '>') {
            t->type = REDIRECT_OUT;
        } else {
            t->type = WORD;
            len = 0;
            while (line[i + len] != '\0' && !isspace((unsigned char)line[i + len]) &&
                   strchr("|&;<>", line[i + len]) == NULL)
                len++;
        }
        snprintf(t->text, sizeof(t->text), "%.*s", len, line + start);
        t->column = start + 1;
        i += len;
    }
    tokens[ntokens].type = END;
    strcpy(tokens[ntokens].text, "end of input");
    return ntokens;
}

static int unexpected(void) {
    struct token *t = &tokens[pos];
    if (t->type == END)
        snprintf(error, sizeof(error), "syntax error: unexpected end of input after `%s'",
                 pos > 0 ? tokens[pos - 1].text : "");
    else
        snprintf(error, sizeof(error), "syntax error near unexpected token `%s' (column %d)",
                 t->text, t->column);
    return -1;
}

/* command := ( WORD | redirect )+ */
static int parse_command(struct command *c) {
    c->argc = 0;
    c->nredirs = 0;
    while (tokens[pos].type == WORD || tokens[pos].type == REDIRECT_IN ||
           tokens[pos].type == REDIRECT_OUT || tokens[pos].type == APPEND) {
        if (tokens[pos].type == WORD) {
            if (c->argc < MAX_ARGS - 1)
                c->argv[c->argc++] = tokens[pos].text;
            pos++;
        } else {
            struct token *op = &tokens[pos++];
            if (tokens[pos].type != WORD)
                return unexpected();
            if (c->nredirs < MAX_REDIRS) {
                c->redirs[c->nredirs] = op;
                c->files[c->nredirs++] = tokens[pos].text;
            }
            pos++;
        }
    }
    if (c->argc == 0)
        return unexpected();
    c->argv[c->argc] = NULL;
    return 0;
}

/* pipeline := command ( "|" command )* */
static int parse_pipeline(struct pipeline *p) {
    p->count = 0;
    if (parse_command(&p->cmds[p->count++]) < 0)
        return -1;
    while (tokens[pos].type == PIPE && p->count < MAX_CMDS) {
        pos++;
        if (parse_command(&p->cmds[p->count++]) < 0)
            return -1;
    }
    return 0;
}

/* list := pipeline ( (";" | "&&" | "||" | "&") pipeline )* [ ";" | "&" ] */
static int parse_list(struct list *l) {
    l->count = 0;
    while (l->count < MAX_PIPELINES) {
        if (parse_pipeline(&l->pipelines[l->count]) < 0)
            return -1;
        l->ops[l->count] = NULL;
        l->count++;

        enum token_type t = tokens[pos].type;
        if (t == END)
            return 0;
        if (t != SEMICOLON && t != AND && t != OR && t != BACKGROUND)
            return unexpected();
        l->ops[l->count - 1] = tokens[pos].text;
        pos++;
        if (tokens[pos].type == END && (t == SEMICOLON || t == BACKGROUND))
            return 0;
    }
    return unexpected();
}

static void print_tree(struct list *l) {
    printf("LIST\n");
    for (int i = 0; i < l->count; i++) {
        struct pipeline *p = &l->pipelines[i];
        printf("  PIPELINE\n");
        for (int j = 0; j < p->count; j++) {
            struct command *c = &p->cmds[j];
            printf("    COMMAND argv=[");
            for (int k = 0; k < c->argc; k++)
                printf("%s%s", k ? ", " : "", c->argv[k]);
            printf("] redirects=[");
            for (int k = 0; k < c->nredirs; k++)
                printf("%s%s %s", k ? ", " : "", c->redirs[k]->text, c->files[k]);
            printf("]\n");
        }
        if (l->ops[i] != NULL)
            printf("  OPERATOR %s\n", l->ops[i]);
    }
}

int main(void) {
    char line[1024];
    struct list *tree = malloc(sizeof(struct list));
    if (tree == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    while (1) {
        printf("parse> ");
        fflush(stdout);
        if (fgets(line, sizeof(line), stdin) == NULL) {
            printf("\n");
            break;
        }
        line[strcspn(line, "\n")] = '\0';
        if (strcmp(line, "exit") == 0)
            break;
        if (tokenize(line) == 0)
            continue;

        pos = 0;
        if (parse_list(tree) < 0)
            printf("%s\n", error);
        else
            print_tree(tree);
    }

    free(tree);
    return 0;
}
