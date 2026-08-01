#include<stdio.h>
#include<unistd.h>
#include<sys/wait.h>
	int main(){
		pid_t pid;
		pid = fork();
		if(pid==0){
			printf("Child PID=%d\n",getpid());
			printf("Parent PID=%d\n",getppid());
			printf("Child Running\n");
			}
		else{
			wait(NULL);
			printf("Parent PID=%d\n",getpid());
			printf("Parent Running After Child\n");
			
			}
		return 0;
		}
