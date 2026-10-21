#include <stdio.h>
#include <stdlib.h>

extern char **environ;

int init_global = 5;
int uninit_global;

struct item {
    const char *segment;
    const char *name;
    void *addr;
};

static int by_address(const void *a, const void *b) {
    const struct item *x = a;
    const struct item *y = b;
    return (x->addr > y->addr) - (x->addr < y->addr);
}

int main(void) {
    int local_var = 1;
    const char *literal = "hello";
    char *heap = malloc(64);
    if (heap == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    struct item items[] = {
        {"text", "main()", (void *)main},
        {"rodata", "\"hello\"", (void *)literal},
        {"data", "init_global", &init_global},
        {"bss", "uninit_global", &uninit_global},
        {"heap", "malloc(64)", heap},
        {"stack", "local_var", &local_var},
        {"env", "environ[0]", environ[0]},
    };
    int n = sizeof(items) / sizeof(items[0]);
    qsort(items, n, sizeof(items[0]), by_address);

    printf("%-8s %-16s %s\n", "SEGMENT", "ADDRESS", "NAME");
    for (int i = 0; i < n; i++)
        printf("%-8s %-16p %s\n", items[i].segment, items[i].addr, items[i].name);

    FILE *fp = fopen("/proc/self/maps", "r");
    if (fp == NULL) {
        perror("fopen");
        free(heap);
        exit(EXIT_FAILURE);
    }

    printf("\nRegions from /proc/self/maps that hold them:\n");
    char line[512];
    while (fgets(line, sizeof(line), fp) != NULL) {
        unsigned long start, end;
        char perms[8];
        char path[256] = "[anonymous]";
        if (sscanf(line, "%lx-%lx %7s %*s %*s %*s %255s", &start, &end, perms, path) < 3)
            continue;
        for (int i = 0; i < n; i++) {
            unsigned long addr = (unsigned long)items[i].addr;
            if (addr >= start && addr < end)
                printf("%-14s %lx-%lx %s %s\n", items[i].name, start, end, perms, path);
        }
    }

    fclose(fp);
    free(heap);
    return 0;
}
