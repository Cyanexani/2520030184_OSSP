#include <stdio.h>

struct Process {
    int pid;
    int arrival_time;
    int burst_time;
    int priority;
    int completion_time;
    int waiting_time;
    int turnaround_time;
    int done;
};

int main(void) {
    int n;
    printf("Enter number of processes: ");
    scanf("%d", &n);

    struct Process p[n];
    for (int i = 0; i < n; i++) {
        p[i].pid = i + 1;
        p[i].done = 0;
        printf("Process %d - Arrival Time: ", p[i].pid);
        scanf("%d", &p[i].arrival_time);
        printf("Process %d - Burst Time: ", p[i].pid);
        scanf("%d", &p[i].burst_time);
        printf("Process %d - Priority (lower = higher priority): ", p[i].pid);
        scanf("%d", &p[i].priority);
    }

    int completed = 0, clock = 0;
    float total_wt = 0, total_tat = 0;

    while (completed < n) {
        int idx = -1;

        for (int i = 0; i < n; i++) {
            if (p[i].done || p[i].arrival_time > clock)
                continue;
            if (idx == -1 || p[i].priority < p[idx].priority ||
                (p[i].priority == p[idx].priority &&
                 p[i].arrival_time < p[idx].arrival_time)) {
                idx = i;
            }
        }

        if (idx == -1) {
            clock++;
            continue;
        }

        clock += p[idx].burst_time;
        p[idx].completion_time = clock;
        p[idx].turnaround_time = p[idx].completion_time - p[idx].arrival_time;
        p[idx].waiting_time = p[idx].turnaround_time - p[idx].burst_time;
        p[idx].done = 1;
        completed++;

        total_wt += p[idx].waiting_time;
        total_tat += p[idx].turnaround_time;
    }

    printf("\nPID\tArrival\tBurst\tPriority\tCompletion\tTurnaround\tWaiting\n");
    for (int i = 0; i < n; i++) {
        printf("%d\t%d\t%d\t%d\t\t%d\t\t%d\t\t%d\n", p[i].pid,
               p[i].arrival_time, p[i].burst_time, p[i].priority,
               p[i].completion_time, p[i].turnaround_time,
               p[i].waiting_time);
    }

    printf("\nAverage Waiting Time: %.2f\n", total_wt / n);
    printf("Average Turnaround Time: %.2f\n", total_tat / n);

    return 0;
}
