#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 5555
#define BUF_SIZE 256

int main(void) {
    int sock_fd;
    struct sockaddr_in addr;
    char buf[BUF_SIZE];

    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    if (connect(sock_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("connect");
        exit(EXIT_FAILURE);
    }

    printf("Connected to server. Type messages ('quit' to exit).\n");
    while (fgets(buf, sizeof(buf), stdin)) {
        buf[strcspn(buf, "\n")] = '\0';
        write(sock_fd, buf, strlen(buf));

        if (strcmp(buf, "quit") == 0)
            break;

        ssize_t n = read(sock_fd, buf, sizeof(buf) - 1);
        if (n <= 0)
            break;
        buf[n] = '\0';
        printf("[server said]: %s\n", buf);
    }

    close(sock_fd);
    return 0;
}
