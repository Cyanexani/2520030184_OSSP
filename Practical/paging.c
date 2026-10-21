#include <stdio.h>

int main(void) {
    int page_size, pages, count;

    printf("Enter page size (bytes): ");
    if (scanf("%d", &page_size) != 1 || page_size <= 0) {
        printf("Invalid page size\n");
        return 1;
    }
    printf("Enter number of pages: ");
    if (scanf("%d", &pages) != 1 || pages <= 0) {
        printf("Invalid number of pages\n");
        return 1;
    }

    int page_table[pages];
    printf("Enter frame number of each page (-1 if not in memory)\n");
    for (int i = 0; i < pages; i++) {
        printf("  Page %d: ", i);
        if (scanf("%d", &page_table[i]) != 1)
            return 1;
    }

    printf("Enter number of logical addresses: ");
    if (scanf("%d", &count) != 1 || count <= 0)
        return 1;
    int addr[count];
    printf("Enter the logical addresses: ");
    for (int i = 0; i < count; i++) {
        if (scanf("%d", &addr[i]) != 1)
            return 1;
    }

    printf("\nLogical  Page  Offset  Frame  Physical\n");
    for (int i = 0; i < count; i++) {
        int page = addr[i] / page_size;
        int offset = addr[i] % page_size;

        if (addr[i] < 0 || page >= pages)
            printf("%-8d %-5s %-7s %-6s invalid address\n", addr[i], "-", "-", "-");
        else if (page_table[page] == -1)
            printf("%-8d %-5d %-7d %-6s page fault\n", addr[i], page, offset, "-");
        else
            printf("%-8d %-5d %-7d %-6d %d\n", addr[i], page, offset, page_table[page],
                   page_table[page] * page_size + offset);
    }
    return 0;
}
