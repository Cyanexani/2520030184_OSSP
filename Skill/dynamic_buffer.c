#include <stdio.h>
#include <stdlib.h>

#define START_SIZE 8

struct line_node {
    char *text;
    size_t len;
    struct line_node *next;
};

static size_t peak_capacity = 0;

/* Reads one line of any length, doubling the buffer whenever it is full */
static char *read_line(size_t *out_len) {
    size_t cap = START_SIZE;
    size_t len = 0;
    int c;
    char *buf = malloc(cap);
    if (buf == NULL)
        return NULL;

    while ((c = getchar()) != EOF && c != '\n') {
        if (len + 1 == cap) {
            char *bigger = realloc(buf, cap * 2);
            if (bigger == NULL) {
                free(buf);
                return NULL;
            }
            printf("  buffer grown: %zu -> %zu bytes\n", cap, cap * 2);
            buf = bigger;
            cap *= 2;
        }
        buf[len++] = c;
    }
    if (c == EOF && len == 0) {
        free(buf);
        return NULL;
    }

    buf[len] = '\0';
    if (cap > peak_capacity)
        peak_capacity = cap;
    *out_len = len;
    return buf;
}

int main(void) {
    struct line_node *head = NULL;
    struct line_node *tail = NULL;
    int lines = 0;
    size_t bytes = 0;
    size_t len;
    char *text;

    printf("Type lines of any length, Ctrl-D to finish.\n");
    while ((text = read_line(&len)) != NULL) {
        struct line_node *node = malloc(sizeof(struct line_node));
        if (node == NULL) {
            perror("malloc");
            free(text);
            break;
        }
        node->text = text;
        node->len = len;
        node->next = NULL;
        if (tail == NULL)
            head = node;
        else
            tail->next = node;
        tail = node;
        lines++;
        bytes += len;
    }

    printf("\nStored lines (linked list):\n");
    int i = 1;
    for (struct line_node *n = head; n != NULL; n = n->next)
        printf("%2d  len %-3zu %s\n", i++, n->len, n->text);
    printf("\nLines: %d, bytes stored: %zu, largest buffer: %zu bytes\n", lines, bytes, peak_capacity);

    int freed = 0;
    while (head != NULL) {
        struct line_node *next = head->next;
        free(head->text);
        free(head);
        head = next;
        freed++;
    }
    printf("Freed %d nodes\n", freed);
    return 0;
}
