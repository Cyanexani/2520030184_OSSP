#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>

#define CHILDREN 4

static struct timespec start;

static double elapsed(void) {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (now.tv_sec - start.tv_sec) + (now.tv_nsec - start.tv_nsec) / 1e9;
}

static pid_t spawn(char label) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    if (pid > 0)
        return pid;

    switch (label) {
    case 'A':
        sleep(1);
        _exit(0);
    case 'B':
        sleep(2);
        _exit(3);
    case 'C':
        sleep(10);
        _exit(0);
    default:
        raise(SIGSTOP);
        sleep(1);
        _exit(0);
    }
}

int main(void) {
    const char labels[CHILDREN] = {'A', 'B', 'C', 'D'};
    pid_t pids[CHILDREN];
    char result[CHILDREN][64];
    int status, alive = CHILDREN, killed = 0;

    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < CHILDREN; i++)
        pids[i] = spawn(labels[i]);
    printf("A %d exits 0 after 1s, B %d exits 3 after 2s, C %d gets killed, D %d stops itself\n\n",
           pids[0], pids[1], pids[2], pids[3]);

    /* WNOHANG returns 0 straight away while no child has finished */
    for (int i = 0; i < 3; i++) {
        if (waitpid(-1, &status, WNOHANG) == 0)
            printf("%.1fs  poll: no child has finished yet\n", elapsed());
        usleep(150000);
    }

    while (alive > 0) {
        if (!killed && elapsed() >= 1.5) {
            kill(pids[2], SIGKILL);
            printf("%.1fs  parent sends SIGKILL to C\n", elapsed());
            killed = 1;
        }

        pid_t pid = waitpid(-1, &status, WUNTRACED | WCONTINUED | WNOHANG);
        if (pid == 0) {
            usleep(20000);
            continue;
        }
        if (pid < 0) {
            perror("waitpid");
            break;
        }

        int i = 0;
        while (i < CHILDREN && pids[i] != pid)
            i++;
        printf("%.1fs  %c (%d) ", elapsed(), labels[i], pid);

        if (WIFEXITED(status)) {
            printf("exited with status %d\n", WEXITSTATUS(status));
            snprintf(result[i], sizeof(result[i]), "exited, status %d", WEXITSTATUS(status));
            alive--;
        } else if (WIFSIGNALED(status)) {
            printf("killed by signal %d (%s)\n", WTERMSIG(status), strsignal(WTERMSIG(status)));
            snprintf(result[i], sizeof(result[i]), "killed by signal %d", WTERMSIG(status));
            alive--;
        } else if (WIFSTOPPED(status)) {
            printf("stopped by signal %d, parent sends SIGCONT\n", WSTOPSIG(status));
            kill(pid, SIGCONT);
        } else if (WIFCONTINUED(status)) {
            printf("continued\n");
        }
    }

    printf("\nChild  PID     Result\n");
    for (int i = 0; i < CHILDREN; i++)
        printf("%-6c %-7d %s\n", labels[i], pids[i], result[i]);
    return 0;
}
