#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#define BUF_SIZE 4096

int main(int argc, char *argv[]) {
    const char *src = argc > 1 ? argv[1] : "note.txt";
    const char *dst = argc > 2 ? argv[2] : "note1.txt";
    char buf[BUF_SIZE];
    struct stat st, dst_st;
    ssize_t n;
    long total = 0;

    int in = open(src, O_RDONLY);
    if (in == -1) {
        perror(src);
        exit(EXIT_FAILURE);
    }
    if (fstat(in, &st) == -1) {
        perror("fstat");
        exit(EXIT_FAILURE);
    }
    if (!S_ISREG(st.st_mode)) {
        printf("%s is not a regular file\n", src);
        exit(EXIT_FAILURE);
    }
    if (stat(dst, &dst_st) == 0 && dst_st.st_dev == st.st_dev && dst_st.st_ino == st.st_ino) {
        printf("%s and %s are the same file\n", src, dst);
        exit(EXIT_FAILURE);
    }

    int out = open(dst, O_WRONLY | O_CREAT | O_TRUNC, st.st_mode & 0777);
    if (out == -1) {
        perror(dst);
        exit(EXIT_FAILURE);
    }

    while ((n = read(in, buf, sizeof(buf))) > 0) {
        ssize_t done = 0;
        while (done < n) {
            ssize_t w = write(out, buf + done, n - done);
            if (w == -1) {
                perror("write");
                exit(EXIT_FAILURE);
            }
            done += w;
        }
        total += n;
    }
    if (n == -1)
        perror("read");
    close(in);
    close(out);
    printf("Copied %ld bytes from %s to %s\n", total, src, dst);

    out = open(dst, O_RDONLY);
    if (out == -1) {
        perror(dst);
        exit(EXIT_FAILURE);
    }
    printf("\n--- %s ---\n", dst);
    while ((n = read(out, buf, sizeof(buf))) > 0)
        printf("%.*s", (int)n, buf);
    printf("---------------------\n");
    close(out);
    return 0;
}
