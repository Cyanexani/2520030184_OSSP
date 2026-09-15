#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MSG_SIZE 256

int main(void) {
    int parent_to_child[2];
    int child_to_parent[2];

    if (pipe(parent_to_child) == -1 || pipe(child_to_parent) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        close(parent_to_child[1]);
        close(child_to_parent[0]);

        char buf[MSG_SIZE];
        ssize_t n = read(parent_to_child[0], buf, sizeof(buf) - 1);
        buf[n] = '\0';
        printf("[child %d] received: %s\n", getpid(), buf);

        const char *ack = "message received";
        write(child_to_parent[1], ack, strlen(ack) + 1);

        close(parent_to_child[0]);
        close(child_to_parent[1]);
        exit(EXIT_SUCCESS);
    } else {
        close(parent_to_child[0]);
        close(child_to_parent[1]);

        const char *msg = "hello from parent";
        write(parent_to_child[1], msg, strlen(msg) + 1);

        char buf[MSG_SIZE];
        ssize_t n = read(child_to_parent[0], buf, sizeof(buf) - 1);
        buf[n] = '\0';
        printf("[parent %d] child replied: %s\n", getpid(), buf);

        close(parent_to_child[1]);
        close(child_to_parent[0]);
        wait(NULL);
    }

    return 0;
}
