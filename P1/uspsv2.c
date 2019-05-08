#include "stdio.h"
#include "stdlib.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <fcntl.h>
#include "p1fxns.h"

/* 	Steve Kent, fkent, CIS 415 P1
This is my own work except for the acknowledgements in the following statement: 
I would like to give credit to Professor Joe Sventek and Roscoe Casita. I used examples in 
Elagent C Programming (written by Sventek) to implement portions of my code. Also, I used 
Sventek's queue, iterator, and p1fxns files to provide my code with further support. I made 
small modifications in p1fxns.c in the p1getword function (labeled within code). From Roscoe, 
I used some of his functions from labs to implement portions of my code. Specifically, I 
adapted functions from lab3, lab4 and lab5.
 */

#define UNUSED __attribute__((unused))

volatile int notify = 0;

typedef struct pcb{
	pid_t child;
} PCB;

PCB pcbs[250];

void addToReady(pid_t child, int idx){
	pcbs[idx].child = child;
}

struct timespec holdup = {0, 50000000L};

void sighand(UNUSED int sig){
	notify = 1;
}
	
char **arraydup(char *array[]){
	int length = 0;
	int i;
	while (array[length] != NULL){
		length++;
	}
	char **temp = NULL;
	temp = malloc(sizeof(char*) * length);
	for (i=0; i<length; i++){
		temp[i] = array[i];
	}
	temp[length] = NULL;
	return temp;
}

//Destroys array.
void arrayfree(char **array){
	int size = 0;
	int i;
	if (array != NULL){
		while(array[size] != NULL){
			size++;
		}
		for (i=0; i<size; i++){
			free(array[i]);
		}
		free(array);	
	}
}

//Calls fork, waits for sigusr1 and execvp's command
pid_t processor(char *arg, char **args){	
	pid_t child;
	child = fork();
	if (child == 0){
		while (!notify){
			nanosleep(&holdup, NULL);
		}
		execvp(arg, args);
		p1putstr(STDERR_FILENO, "execvp failed\n");
		arrayfree(args);
		_exit(0);
	}	
	return child;
}

//Calls waitpid
int waiter(pid_t pid){
	int status;
	waitpid(pid, &status, 0);
	return status;

}

//Counts number of words in sentence
int wordcount(char sentence[]){
	int len = p1strlen(sentence);
	int idx = 0;
	int count = 0;
	char unused[1024];
	while (idx != -1 && idx < len-1){
		idx = p1getword(sentence, idx, unused);
		count++;
	}
	return count;
}

//Duplicates buffer to heap, call p1getword.
char **bufreader(char buffer[]){
	int len = p1strlen(buffer);
	int words = wordcount(buffer);
	int idx = 0;
	int i = 0;
	char **args = malloc(sizeof(char*) * (words+1));
	char temp[1024];
	while (idx != 1 && idx < len-1){
		idx = p1getword(buffer, idx, temp);
		args[i] = p1strdup(temp);
		i++;
	}
	args[i] = NULL;			
	return args;

}



int main(int argc, char *argv[]){
	char *q = "--quantum=";
	int fd;
	int qpresent = 0;
	int epresent = 0;
	char *env;
	int quantum = -1;
	if (argc == 1){
		p1putstr(STDERR_FILENO, "usage: ./uspsv? [–-quantum=<msec>] [workload_file]\n");
		_exit(0);
	}
	if (argc > 4){
		p1putstr(STDERR_FILENO, "usage: ./uspsv? [–-quantum=<msec>] [workload_file]\n");
		_exit(0);
	}
	if ((env = getenv("USPS_QUANTUM_MSEC")) != NULL){
		epresent = 1;
		quantum = p1atoi(env);
	}
	int len = p1strlen(q);
	if(p1strneq(argv[1], q, len)){
		qpresent = 1;
		quantum = p1atoi(argv[1]+10);
	}
	if (quantum == -1 && !(qpresent) && !(epresent)){
		p1putstr(STDERR_FILENO, "usage: ./uspsv? [–-quantum=<msec>] [workload_file]\n");
		_exit(0);
	} 
	
	if (signal(SIGUSR1, sighand) == SIG_ERR){
				_exit(0);
	}
	
	if (qpresent){	
		if ((fd = open(argv[2], 0)) == -1){
			//standard input, quantum present
			char **adup = arraydup(&argv[2]);
			pid_t child;
			child = processor(adup[0], &adup[0]);		
			kill(child, SIGUSR1);
			waiter(child);
			
		} else {
			//file input, quantum present
			char buffer[1024];	
			int cnter = 0;
			pid_t child;

			while (p1getline(fd, buffer, 1024) != 0){		
				char **args = bufreader(buffer);	
				child = processor(args[0], &args[0]);
				addToReady(child, cnter);
				cnter++;
				arrayfree(args);
							
			}					
			int i;
			for (i=0; i<cnter; i++){
				kill(pcbs[i].child, SIGUSR1);				 
			}
	
			for (i=0; i<cnter; i++){
				kill(pcbs[i].child, SIGSTOP);
			}
			
			for (i=0; i<cnter; i++){	
				kill(pcbs[i].child, SIGCONT);
			}	
			for (i=0; i<cnter; i++){
				waiter(pcbs[i].child);
			}		
		}
	} else {
		if ((fd = open(argv[1], 0)) == -1){
			//standard input, quantum NOT present
			char **adup = arraydup(&argv[1]);
			pid_t child;
			child = processor(adup[0], &adup[0]);		
			kill(child, SIGUSR1);
			waiter(child);
		} else {
			//file input, quantum NOT present in args
			char buffer[1024];	
			int cnter = 0;
			pid_t child;
			while (p1getline(fd, buffer, 1024) != 0){		
				char **args = bufreader(buffer);	
				child = processor(args[0], &args[0]);
				addToReady(child, cnter);
				cnter++;
				arrayfree(args);
							
			}					
			int i;
			for (i=0; i<cnter; i++){
				kill(pcbs[i].child, SIGUSR1);				 
			}
	
			for (i=0; i<cnter; i++){
				kill(pcbs[i].child, SIGSTOP);
			}
			for (i=0; i<cnter; i++){	
				kill(pcbs[i].child, SIGCONT);
			}	
			for (i=0; i<cnter; i++){
				waiter(pcbs[i].child);
			}		
		}
	}
	return 0;
}

