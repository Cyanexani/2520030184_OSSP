#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define FILE_NAME "demo_file.txt"
#define BUF_SIZE 256

int main(void) {
    int fd;
    char buf[BUF_SIZE];
    ssize_t n;

    fd = open(FILE_NAME, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd == -1) {
        perror("open (create)");
        exit(EXIT_FAILURE);
    }
    const char *line1 = "This is the first line.\n";
    write(fd, line1, strlen(line1));
    close(fd);
    printf("Created %s and wrote initial content.\n", FILE_NAME);

    fd = open(FILE_NAME, O_WRONLY | O_APPEND);
    if (fd == -1) {
        perror("open (append)");
        exit(EXIT_FAILURE);
    }
    const char *line2 = "This is an appended line.\n";
    write(fd, line2, strlen(line2));
    close(fd);
    printf("Appended a second line.\n");

    fd = open(FILE_NAME, O_RDONLY);
    if (fd == -1) {
        perror("open (read)");
        exit(EXIT_FAILURE);
    }

    printf("\n--- File contents ---\n");
    while ((n = read(fd, buf, sizeof(buf) - 1)) > 0) {
        buf[n] = '\0';
        printf("%s", buf);
    }
    printf("---------------------\n");
    close(fd);

    return 0;
}
