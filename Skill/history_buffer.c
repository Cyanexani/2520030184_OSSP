#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define CAPACITY 5

struct history {
    char *entries[CAPACITY];
    int start;
    int count;
    long total;
};

static void add_entry(struct history *h, const char *line) {
    if (h->count == CAPACITY) {
        printf("evicted #%ld: %s\n", h->total - CAPACITY + 1, h->entries[h->start]);
        free(h->entries[h->start]);
        h->entries[h->start] = strdup(line);
        h->start = (h->start + 1) % CAPACITY;
    } else {
        h->entries[(h->start + h->count) % CAPACITY] = strdup(line);
        h->count++;
    }
    h->total++;
    printf("stored #%ld: %s\n", h->total, line);
}

/* number is the absolute command number, counting from 1 */
static const char *get_entry(struct history *h, long number) {
    long first = h->total - h->count + 1;
    if (number < first || number > h->total)
        return NULL;
    return h->entries[(h->start + (number - first)) % CAPACITY];
}

static const char *recall(struct history *h, const char *event) {
    if (strcmp(event, "!!") == 0)
        return get_entry(h, h->total);
    if (event[1] == '-' && isdigit((unsigned char)event[2]))
        return get_entry(h, h->total - atol(event + 2) + 1);
    if (isdigit((unsigned char)event[1]))
        return get_entry(h, atol(event + 1));

    for (long n = h->total; n > h->total - h->count; n--) {
        const char *e = get_entry(h, n);
        if (strncmp(e, event + 1, strlen(event + 1)) == 0)
            return e;
    }
    return NULL;
}

int main(void) {
    struct history h = {0};
    char line[1024];

    printf("History keeps the last %d commands.\n", CAPACITY);
    while (1) {
        printf("history> ");
        fflush(stdout);
        if (fgets(line, sizeof(line), stdin) == NULL) {
            printf("\n");
            break;
        }
        line[strcspn(line, "\n")] = '\0';
        if (line[0] == '\0')
            continue;
        if (strcmp(line, "exit") == 0)
            break;

        if (strcmp(line, "history") == 0) {
            for (long n = h.total - h.count + 1; n <= h.total; n++)
                printf("%4ld  %s\n", n, get_entry(&h, n));
        } else if (line[0] == '!' && line[1] != '\0') {
            const char *found = recall(&h, line);
            if (found == NULL) {
                printf("%s: event not found\n", line);
                continue;
            }
            char *copy = strdup(found);
            printf("recalled: %s\n", copy);
            add_entry(&h, copy);
            free(copy);
        } else {
            add_entry(&h, line);
        }
    }

    for (int i = 0; i < h.count; i++)
        free(h.entries[(h.start + i) % CAPACITY]);
    return 0;
}
