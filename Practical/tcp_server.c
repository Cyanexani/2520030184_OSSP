#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 5555
#define BUF_SIZE 256

int main(void) {
    int server_fd, client_fd;
    struct sockaddr_in addr;
    char buf[BUF_SIZE];
    int opt = 1;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 1) == -1) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("[server] listening on port %d\n", PORT);

    socklen_t addrlen = sizeof(addr);
    client_fd = accept(server_fd, (struct sockaddr *)&addr, &addrlen);
    if (client_fd == -1) {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    printf("[server] client connected: %s\n", inet_ntoa(addr.sin_addr));

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
    return 0;
}
