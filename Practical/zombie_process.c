#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void) {
    char cmd[64];
    int status;

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    if (pid == 0) {
        printf("Child %d exiting with status 7\n", getpid());
        exit(7);
    }

    sleep(2);
    snprintf(cmd, sizeof(cmd), "ps -o pid,ppid,stat,cmd -p %d", pid);
    printf("\nChild has exited but the parent has not called wait() yet:\n");
    fflush(stdout);
    if (system(cmd) == -1)
        perror("system");

    waitpid(pid, &status, 0);
    printf("\nParent reaped child %d, exit status %d\n", pid, WEXITSTATUS(status));
    fflush(stdout);
    if (system(cmd) == -1)
        perror("system");
    printf("The zombie entry is gone from the process table\n");
    return 0;
}
