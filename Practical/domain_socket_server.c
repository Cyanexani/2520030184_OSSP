#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_PATH "/tmp/ossp_domain.sock"
#define BUF_SIZE 256

int main(void) {
    int server_fd, client_fd;
    struct sockaddr_un addr;
    char buf[BUF_SIZE];

    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    unlink(SOCKET_PATH);

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 1) == -1) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("[server] listening on %s\n", SOCKET_PATH);

    client_fd = accept(server_fd, NULL, NULL);
    if (client_fd == -1) {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    ssize_t n;
    while ((n = read(client_fd, buf, sizeof(buf) - 1)) > 0) {
        buf[n] = '\0';
        if (strcmp(buf, "quit") == 0)
            break;
        for (int i = 0; buf[i]; i++)
            buf[i] = toupper((unsigned char)buf[i]);
        printf("[server] echoing: %s\n", buf);
        write(client_fd, buf, strlen(buf));
    }

    close(client_fd);
    close(server_fd);
    unlink(SOCKET_PATH);
    return 0;
}
