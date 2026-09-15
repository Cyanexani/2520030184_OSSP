#include <stdio.h>

struct Process {
    int pid;
    int arrival_time;
    int burst_time;
    int remaining_time;
    int completion_time;
    int waiting_time;
    int turnaround_time;
    int in_queue;
};

int main(void) {
    int n, quantum;
    printf("Enter number of processes: ");
    scanf("%d", &n);

    struct Process p[n];
    for (int i = 0; i < n; i++) {
        p[i].pid = i + 1;
        p[i].in_queue = 0;
        printf("Process %d - Arrival Time: ", p[i].pid);
        scanf("%d", &p[i].arrival_time);
        printf("Process %d - Burst Time: ", p[i].pid);
        scanf("%d", &p[i].burst_time);
        p[i].remaining_time = p[i].burst_time;
    }

    printf("Enter time quantum: ");
    scanf("%d", &quantum);

    int queue[1000], front = 0, rear = 0;
    int clock = 0, completed = 0;
    float total_wt = 0, total_tat = 0;

    for (int i = 0; i < n; i++) {
        if (p[i].arrival_time <= clock && !p[i].in_queue) {
            queue[rear++] = i;
            p[i].in_queue = 1;
        }
    }

    while (completed < n) {
        if (front == rear) {
            clock++;
            for (int i = 0; i < n; i++) {
                if (p[i].arrival_time <= clock && !p[i].in_queue &&
                    p[i].remaining_time > 0) {
                    queue[rear++] = i;
                    p[i].in_queue = 1;
                }
            }
            continue;
        }

        int idx = queue[front++];
        int run = p[idx].remaining_time < quantum ? p[idx].remaining_time
                                                    : quantum;
        clock += run;
        p[idx].remaining_time -= run;

        for (int i = 0; i < n; i++) {
            if (p[i].arrival_time <= clock && !p[i].in_queue &&
                p[i].remaining_time > 0) {
                queue[rear++] = i;
                p[i].in_queue = 1;
            }
        }

        if (p[idx].remaining_time > 0) {
            queue[rear++] = idx;
        } else {
            p[idx].completion_time = clock;
            p[idx].turnaround_time =
                p[idx].completion_time - p[idx].arrival_time;
            p[idx].waiting_time = p[idx].turnaround_time - p[idx].burst_time;
            total_wt += p[idx].waiting_time;
            total_tat += p[idx].turnaround_time;
            completed++;
        }
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
