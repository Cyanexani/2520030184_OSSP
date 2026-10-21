#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>

#define FIFO_PATH "/tmp/ossp_fifo"

int main(void) {
    struct stat st;

    if (mkfifo(FIFO_PATH, 0666) == -1) {
        if (errno != EEXIST) {
            perror("mkfifo");
            exit(EXIT_FAILURE);
        }
        printf("%s already exists\n", FIFO_PATH);
    } else {
        printf("Created named pipe %s\n", FIFO_PATH);
    }

    if (stat(FIFO_PATH, &st) == -1) {
        perror("stat");
        exit(EXIT_FAILURE);
    }
    printf("Type        : %s\n", S_ISFIFO(st.st_mode) ? "FIFO (named pipe)" : "not a FIFO");
    printf("Permissions : %o\n", st.st_mode & 0777);
    return 0;
}
