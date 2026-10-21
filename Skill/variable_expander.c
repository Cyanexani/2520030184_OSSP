#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#define MAX_VARS 32
#define MAX_DEPTH 8
#define OUT_SIZE 1024

struct var {
    char name[64];
    char value[256];
};

static struct var vars[MAX_VARS];
static int nvars = 0;
static int last_status = 0;

static const char *lookup(const char *name) {
    for (int i = 0; i < nvars; i++) {
        if (strcmp(vars[i].name, name) == 0)
            return vars[i].value;
    }
    return getenv(name);
}

static int valid_name(const char *s) {
    if (!isalpha((unsigned char)*s) && *s != '_')
        return 0;
    for (s++; *s != '\0'; s++) {
        if (!isalnum((unsigned char)*s) && *s != '_')
            return 0;
    }
    return 1;
}

static void set_var(const char *name, const char *value) {
    int i = 0;
    while (i < nvars && strcmp(vars[i].name, name) != 0)
        i++;
    if (i == MAX_VARS)
        return;
    if (i == nvars)
        nvars++;
    snprintf(vars[i].name, sizeof(vars[i].name), "%.63s", name);
    snprintf(vars[i].value, sizeof(vars[i].value), "%.255s", value);
}

static void add(char *out, size_t *len, const char *s) {
    while (*s != '\0' && *len < OUT_SIZE - 1)
        out[(*len)++] = *s++;
    out[*len] = '\0';
}

static int expand(const char *in, char *out, int depth) {
    size_t len = 0;
    out[0] = '\0';
    if (depth > MAX_DEPTH)
        return -1;

    for (int i = 0; in[i] != '\0';) {
        if (in[i] == '\'') {
            for (i++; in[i] != '\0' && in[i] != '\''; i++) {
                char c[2] = {in[i], '\0'};
                add(out, &len, c);
            }
            if (in[i] == '\'')
                i++;
            continue;
        }
        if (in[i] != '$') {
            char c[2] = {in[i++], '\0'};
            add(out, &len, c);
            continue;
        }

        char name[64] = "", def[256] = "", num[32];
        int has_default = 0, j = 0;
        i++;
        if (in[i] == '$' || in[i] == '?') {
            snprintf(num, sizeof(num), "%d", in[i] == '$' ? (int)getpid() : last_status);
            add(out, &len, num);
            i++;
            continue;
        }
        if (in[i] == '{') {
            for (i++; in[i] != '\0' && in[i] != '}' && in[i] != ':' && j < 63; i++)
                name[j++] = in[i];
            name[j] = '\0';
            if (in[i] == ':' && in[i + 1] == '-') {
                int k = 0;
                for (i += 2; in[i] != '\0' && in[i] != '}' && k < 255; i++)
                    def[k++] = in[i];
                def[k] = '\0';
                has_default = 1;
            }
            if (in[i] == '}')
                i++;
        } else if (isalpha((unsigned char)in[i]) || in[i] == '_') {
            while ((isalnum((unsigned char)in[i]) || in[i] == '_') && j < 63)
                name[j++] = in[i++];
            name[j] = '\0';
        } else {
            add(out, &len, "$");
            continue;
        }

        const char *value = lookup(name);
        if (value == NULL || value[0] == '\0') {
            if (has_default)
                value = def;
            else if (value == NULL)
                fprintf(stderr, "warning: %s is not set\n", name);
            if (value == NULL)
                value = "";
        }

        /* a value can hold more $ references, so expand it again one level deeper */
        if (strchr(value, '$') != NULL) {
            char nested[OUT_SIZE];
            if (expand(value, nested, depth + 1) < 0)
                return -1;
            add(out, &len, nested);
        } else {
            add(out, &len, value);
        }
    }
    return 0;
}

int main(void) {
    char line[1024];
    char out[OUT_SIZE];

    while (1) {
        printf("expand> ");
        fflush(stdout);
        if (fgets(line, sizeof(line), stdin) == NULL) {
            printf("\n");
            break;
        }
        line[strcspn(line, "\n")] = '\0';
        if (strcmp(line, "exit") == 0)
            break;

        if (strncmp(line, "set ", 4) == 0) {
            char *eq = strchr(line + 4, '=');
            if (eq == NULL) {
                printf("usage: set NAME=value\n");
                last_status = 1;
                continue;
            }
            *eq = '\0';
            if (!valid_name(line + 4)) {
                printf("set: `%s': not a valid name\n", line + 4);
                last_status = 1;
                continue;
            }
            set_var(line + 4, eq + 1);
            printf("%s set to '%s'\n", line + 4, eq + 1);
            last_status = 0;
            continue;
        }

        if (expand(line, out, 0) < 0) {
            printf("error: expansion depth exceeded\n");
            last_status = 1;
        } else {
            printf("expanded: %s\n", out);
            last_status = 0;
        }
    }
    return 0;
}
