#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/resource.h>

#define REAL_PAGES 1024

static long minor_faults(void) {
    struct rusage ru;
    getrusage(RUSAGE_SELF, &ru);
    return ru.ru_minflt;
}

int main(void) {
    int frames, n;

    printf("Enter number of frames: ");
    if (scanf("%d", &frames) != 1 || frames <= 0)
        return 1;
    printf("Enter length of reference string: ");
    if (scanf("%d", &n) != 1 || n <= 0)
        return 1;

    int ref[n], mem[frames];
    printf("Enter reference string: ");
    for (int i = 0; i < n; i++) {
        if (scanf("%d", &ref[i]) != 1)
            return 1;
    }
    for (int i = 0; i < frames; i++)
        mem[i] = -1;

    int next = 0, faults = 0;
    printf("\n%-5s%*s  %s\n", "Page", frames * 3, "Frames", "Result");
    for (int i = 0; i < n; i++) {
        int hit = 0;
        for (int j = 0; j < frames; j++) {
            if (mem[j] == ref[i])
                hit = 1;
        }
        if (!hit) {
            mem[next] = ref[i];
            next = (next + 1) % frames;
            faults++;
        }
        printf("%-5d", ref[i]);
        for (int j = 0; j < frames; j++) {
            if (mem[j] == -1)
                printf("  -");
            else
                printf("%3d", mem[j]);
        }
        printf("  %s\n", hit ? "hit" : "FAULT");
    }
    printf("\nPage faults = %d, hits = %d, hit ratio = %.2f%%\n", faults, n - faults,
           100.0 * (n - faults) / n);

    long page_size = sysconf(_SC_PAGESIZE);
    long start = minor_faults();
    char *region = mmap(NULL, REAL_PAGES * page_size, PROT_READ | PROT_WRITE,
                        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (region == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }
    long after_map = minor_faults();
    for (long i = 0; i < REAL_PAGES; i++)
        region[i * page_size] = 1;
    long after_touch = minor_faults();

    printf("\nReal page faults, page size %ld bytes\n", page_size);
    printf("mmap of %d pages        : %ld minor faults\n", REAL_PAGES, after_map - start);
    printf("writing to every page   : %ld minor faults\n", after_touch - after_map);

    munmap(region, REAL_PAGES * page_size);
    return 0;
}
