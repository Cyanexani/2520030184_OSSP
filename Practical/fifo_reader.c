#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#define FIFO_PATH "/tmp/ossp_fifo"
#define BUF_SIZE 256

int main(void) {
    char buf[BUF_SIZE];

    if (mkfifo(FIFO_PATH, 0666) == -1 && errno != EEXIST) {
        perror("mkfifo");
        exit(EXIT_FAILURE);
    }

    printf("[reader] waiting for a writer on %s\n", FIFO_PATH);
    fflush(stdout);
    int fd = open(FIFO_PATH, O_RDONLY);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }
    FILE *in = fdopen(fd, "r");
    if (in == NULL) {
        perror("fdopen");
        close(fd);
        exit(EXIT_FAILURE);
    }
    printf("[reader] connected\n");

    while (fgets(buf, sizeof(buf), in) != NULL)
        printf("[reader] received: %s", buf);

    fclose(in);
    printf("[reader] writer closed the FIFO\n");
    return 0;
}
