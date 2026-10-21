#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

static void print_array(const char *label, int *a, int n) {
    printf("%-8s", label);
    for (int i = 0; i < n; i++)
        printf(" %d", a[i]);
    printf("\n");
}

int main(void) {
    int n;
    printf("Enter number of elements: ");
    if (scanf("%d", &n) != 1 || n <= 0)
        n = 5;

    int *arr = malloc(n * sizeof(int));
    if (arr == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < n; i++)
        arr[i] = (i + 1) * 10;
    printf("\nmalloc: %d ints at %p\n", n, (void *)arr);
    print_array("arr:", arr, n);

    int *zeros = calloc(n, sizeof(int));
    if (zeros == NULL) {
        perror("calloc");
        free(arr);
        exit(EXIT_FAILURE);
    }
    printf("\ncalloc: %d ints at %p, memory starts zeroed\n", n, (void *)zeros);
    print_array("zeros:", zeros, n);

    char old_addr[32];
    snprintf(old_addr, sizeof(old_addr), "%p", (void *)arr);
    int *bigger = realloc(arr, 2 * n * sizeof(int));
    if (bigger == NULL) {
        perror("realloc");
        free(arr);
        free(zeros);
        exit(EXIT_FAILURE);
    }
    int moved = bigger != arr;
    arr = bigger;
    for (int i = n; i < 2 * n; i++)
        arr[i] = (i + 1) * 10;
    printf("\nrealloc to %d ints: %s -> %p (%s)\n", 2 * n, old_addr, (void *)arr,
           moved ? "moved, old values copied" : "grown in place");
    print_array("arr:", arr, 2 * n);

    void *p = malloc(SIZE_MAX / 2);
    if (p == NULL)
        printf("\nmalloc(SIZE_MAX / 2) returned NULL, the request was refused\n");
    free(p);

    free(arr);
    free(zeros);
    printf("\nAll blocks freed\n");
    return 0;
}
