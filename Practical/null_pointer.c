#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>

static void segv_handler(int sig, siginfo_t *info, void *context) {
    char msg[64] = "Caught SIGSEGV, faulting address 0x";
    char hex[20];
    unsigned long addr = (unsigned long)info->si_addr;
    int len = strlen(msg);
    int h = 0;

    (void)sig;
    (void)context;
    /* printf is not safe inside a signal handler, so build the text by hand */
    do {
        hex[h++] = "0123456789abcdef"[addr % 16];
        addr /= 16;
    } while (addr > 0);
    while (h > 0)
        msg[len++] = hex[--h];
    msg[len++] = '\n';
    if (write(STDOUT_FILENO, msg, len) == -1)
        _exit(EXIT_FAILURE);
    _exit(EXIT_SUCCESS);
}

int main(void) {
    int status;
    int *volatile p = NULL;

    printf("Child process writes to a NULL pointer\n");
    fflush(stdout);
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    if (pid == 0) {
        *p = 100;
        printf("Child still running\n");
        exit(EXIT_SUCCESS);
    }

    waitpid(pid, &status, 0);
    if (WIFSIGNALED(status))
        printf("Child %d was killed by signal %d (%s)\n", pid, WTERMSIG(status),
               strsignal(WTERMSIG(status)));

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = segv_handler;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGSEGV, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    printf("\nParent installed a SIGSEGV handler and writes to a NULL pointer\n");
    fflush(stdout);
    *p = 100;
    printf("This line is never printed\n");
    return 0;
}
