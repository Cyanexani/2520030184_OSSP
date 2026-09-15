#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_PATH "/tmp/ossp_domain.sock"
#define BUF_SIZE 256

int main(void) {
    int sock_fd;
    struct sockaddr_un addr;
    char buf[BUF_SIZE];

    sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (connect(sock_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("connect");
        exit(EXIT_FAILURE);
    }

    printf("Connected. Type messages ('quit' to exit).\n");
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
