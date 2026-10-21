#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int global_var = 10;

int main(void) {
    static int static_var = 20;
    int local_var = 30;
    int *heap_var = malloc(sizeof(int));
    if (heap_var == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    *heap_var = 40;

    printf("Virtual addresses in process %d\n", getpid());
    printf("main()      %p\n", (void *)main);
    printf("global_var  %p  value = %d\n", (void *)&global_var, global_var);
    printf("static_var  %p  value = %d\n", (void *)&static_var, static_var);
    printf("local_var   %p  value = %d\n", (void *)&local_var, local_var);
    printf("heap_var    %p  value = %d\n", (void *)heap_var, *heap_var);
    fflush(stdout);

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        global_var = 100;
        *heap_var = 400;
        printf("\nChild %d changed global_var and heap_var\n", getpid());
        printf("global_var  %p  value = %d\n", (void *)&global_var, global_var);
        printf("heap_var    %p  value = %d\n", (void *)heap_var, *heap_var);
        free(heap_var);
        exit(EXIT_SUCCESS);
    }

    waitpid(pid, NULL, 0);
    printf("\nParent %d after the child exits\n", getpid());
    printf("global_var  %p  value = %d\n", (void *)&global_var, global_var);
    printf("heap_var    %p  value = %d\n", (void *)heap_var, *heap_var);
    printf("\nSame virtual addresses, different values: each process has its own address space.\n");

    free(heap_var);
    return 0;
}
