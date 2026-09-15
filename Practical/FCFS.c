#include <stdio.h>

struct Process {
    int pid;
    int arrival_time;
    int burst_time;
    int completion_time;
    int waiting_time;
    int turnaround_time;
};

int main(void) {
    int n;
    printf("Enter number of processes: ");
    scanf("%d", &n);

    struct Process p[n];
    for (int i = 0; i < n; i++) {
        p[i].pid = i + 1;
        printf("Process %d - Arrival Time: ", p[i].pid);
        scanf("%d", &p[i].arrival_time);
        printf("Process %d - Burst Time: ", p[i].pid);
        scanf("%d", &p[i].burst_time);
    }

    for (int i = 0; i < n - 1; i++) {
        for (int j = i + 1; j < n; j++) {
            if (p[j].arrival_time < p[i].arrival_time) {
                struct Process tmp = p[i];
                p[i] = p[j];
                p[j] = tmp;
            }
        }
    }

    int clock = 0;
    float total_wt = 0, total_tat = 0;

    for (int i = 0; i < n; i++) {
        if (clock < p[i].arrival_time)
            clock = p[i].arrival_time;
        clock += p[i].burst_time;
        p[i].completion_time = clock;
        p[i].turnaround_time = p[i].completion_time - p[i].arrival_time;
        p[i].waiting_time = p[i].turnaround_time - p[i].burst_time;
        total_wt += p[i].waiting_time;
        total_tat += p[i].turnaround_time;
    }

    printf("\nPID\tArrival\tBurst\tCompletion\tTurnaround\tWaiting\n");
    for (int i = 0; i < n; i++) {
        printf("%d\t%d\t%d\t%d\t\t%d\t\t%d\n", p[i].pid, p[i].arrival_time,
               p[i].burst_time, p[i].completion_time, p[i].turnaround_time,
               p[i].waiting_time);
    }

    printf("\nAverage Waiting Time: %.2f\n", total_wt / n);
    printf("Average Turnaround Time: %.2f\n", total_tat / n);

    return 0;
}
