#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>

#define FIFO_PATH "/tmp/ossp_fifo"
#define BUF_SIZE 256

int main(void) {
    char line[BUF_SIZE];

    if (mkfifo(FIFO_PATH, 0666) == -1 && errno != EEXIST) {
        perror("mkfifo");
        exit(EXIT_FAILURE);
    }
    signal(SIGPIPE, SIG_IGN);

    printf("[writer] waiting for a reader on %s\n", FIFO_PATH);
    fflush(stdout);
    int fd = open(FIFO_PATH, O_WRONLY);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }
    printf("Connected. Type messages ('quit' to exit).\n");

    while (fgets(line, sizeof(line), stdin) != NULL) {
        if (strcmp(line, "quit\n") == 0)
            break;
        if (write(fd, line, strlen(line)) == -1) {
            perror("write");
            break;
        }
    }

    close(fd);
    printf("[writer] closed the FIFO\n");
    return 0;
}
