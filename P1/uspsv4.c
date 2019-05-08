#include "stdio.h"
#include "stdlib.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <signal.h>
#include "p1fxns.h"
#include "queue.h"

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

const Queue *rq = NULL; //ready queue
const Queue *run = NULL; //run queue

typedef struct pcb{
	pid_t child;
	int seen; 
	int dead;
} PCB;

PCB pcbs[250];

int childcount = 0, timercount = 0, exited = 0, notify = 0;

pid_t parent;

void addToReady(const Queue *queue, pid_t child, int idx){
	childcount++;
	pcbs[idx].child = child;
	pcbs[idx].seen = 0;
	pcbs[idx].dead = 0;
	if(!queue->enqueue(queue, child)){
		p1putstr(STDERR_FILENO, "queue error\n");
	}	
}

struct timespec holdup = {0, 50000000L};

void usr1(UNUSED int sig){
	notify = 1;
}

//searches pcb for child, returns index
int getPCB(pid_t child){
	int i;
	for (i=0; i<childcount; i++){
		if (pcbs[i].child == child){
				return i;
		}
	}
	return -1;
}

//signal handler for child, tracks if exited and alerts parent
void sigchild(UNUSED int signal)
{
	sigset_t signal_set; 
	sigemptyset(&signal_set);
	sigaddset(&signal_set, SIGALRM);
	sigprocmask(SIG_BLOCK, &signal_set, NULL);  /* block sig timer signal */
	
	pid_t pid;
	int status;
	while ((pid = waitpid(-1, &status, WNOHANG)) > 0){
		if (WIFEXITED(status) || WIFSIGNALED(status)){
			pcbs[getPCB(pid)].dead = 1;
			exited++;
			//stop the parent pause() using sigusr1 to invoke a signal handler
			kill(parent, SIGUSR1); 
		}
	} 
	
	
	sigprocmask(SIG_UNBLOCK, &signal_set, NULL);  /* reenable sig timer signal */
}

//returns true or false if the child has died
int isdead(pid_t child){
	int pcbidx = getPCB(child);
	if (pcbidx != -1){
		if (pcbs[pcbidx].dead){
			return 1;
		}
	}
	return 0;
}

//returns true or false if the child has seen sigusr1
int isseen(pid_t child){
	int pcbidx = getPCB(child);
	if (pcbidx != -1){
		if (pcbs[pcbidx].seen){
			return 1;
		}
	}
	return 0;
}

//forward declaration
void start_timer(int sec, int msec);

//Signal handler for alarm.
//This is where Round Robin is implemented using a ready queue and running queue.
//Keeps track of timercount so that it can add the first item to run queue and then 
//stop each successive child after the first one.
void sigalarm(UNUSED int signal){
	sigset_t signal_set; 
	sigemptyset(&signal_set);
	sigaddset(&signal_set, SIGCHLD);
	sigprocmask(SIG_BLOCK, &signal_set, NULL);  /* block child exit signal */
	
	pid_t ret1, ret2;
	if (timercount != 0){ // After the timer has fired once there will be something to stop
		run->dequeue(run, (void**)&ret2);
		if (!isdead(ret2)){
			rq->enqueue(rq, ret2);
			kill(ret2, SIGSTOP);
		}
	}
	if (rq->dequeue(rq, (void**)&ret1)){
		run->enqueue(run, ret1);
		if (!isseen(ret1)){
			pcbs[getPCB(ret1)].seen = 1;
			kill(ret1, SIGUSR1);
		} else {
			kill(ret1, SIGCONT);
		}
	}
	timercount++;	
	
	sigprocmask(SIG_UNBLOCK, &signal_set, NULL);  /* reenable child exit signal */
}

//Starts the timer with passed in time values. 
void start_timer(int sec, int msec){
	signal(SIGALRM, sigalarm);
	struct itimerval timer;
	timer.it_interval.tv_sec = sec;
	timer.it_interval.tv_usec = msec;
	timer.it_value.tv_sec = sec;
	timer.it_value.tv_usec = msec;
	setitimer(ITIMER_REAL, &timer, 0);
}
	
//Duplicates standard input on heap.
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
		//free up memory
		pid_t ext;
		rq->dequeue(rq, (void**)&ext);
		run->dequeue(run, (void**)&ext);
		arrayfree(args);
		_exit(0);
	}	
	return child;
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

//Calls waitpid
int waiter(pid_t pid){
	int status;
	waitpid(pid, &status, 0);
	return status;
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
	
	//Setup queues
	if ((rq = Queue_create(0L)) == NULL){
		p1putstr(STDERR_FILENO, "error creating queue\n");
		_exit(0);
	}
	if ((run = Queue_create(0L)) == NULL){
		p1putstr(STDERR_FILENO, "error creating queue\n");
		_exit(0);
	}
	
	//Setup signals
	if (signal(SIGUSR1, usr1) == SIG_ERR){
		_exit(0);
	}

	if (signal(SIGCHLD, sigchild) == SIG_ERR){
		_exit(0);
	}
	if (signal(SIGALRM, sigalarm) == SIG_ERR){
		_exit(0);
	}

	//-----------------------------------------------------------
	if (qpresent){	
		if ((fd = open(argv[2], 0)) == -1){
			//standard input
			char **adup = arraydup(&argv[2]);
			pid_t child;
			child = processor(adup[0], &adup[0]);	
			kill(child, SIGUSR1);
			waiter(child);
			
		} else {
			char buffer[1024];	
			int cnter = 0;
			pid_t child;
			
			while (p1getline(fd, buffer, 1024) != 0){		
				char **args = bufreader(buffer);	
				child = processor(args[0], &args[0]);
				addToReady(rq, child, cnter);
				arrayfree(args);
				cnter++;			
			}
			//save parent pid
			parent = getpid();
			//start the timer
			start_timer(0, quantum*1000);
	
			while (exited != childcount){
				pause();
			}
    		rq->destroy(rq, free);
			pid_t last;
			//I had one too many frees in valgrind
			run->dequeue(run, (void**)&last); 
    		run->destroy(run, free);		
					
		}
	} else {
		if ((fd = open(argv[1], 0)) == -1){
			//standard input
			char **adup = arraydup(&argv[1]);
			pid_t child;
			child = processor(adup[0], &adup[0]);		
			kill(child, SIGUSR1);
			waiter(child);

		} else {
			char buffer[1024];	
			int cnter = 0;
			pid_t child;
			
			while (p1getline(fd, buffer, 1024) != 0){		
				char **args = bufreader(buffer);	
				child = processor(args[0], &args[0]);
				addToReady(rq, child, cnter);
				arrayfree(args);
				cnter++;			
			}
			//save parent pid
			parent = getpid();
			//start the timer
			start_timer(0, quantum*1000);
	
			while (exited != childcount){
				pause();
			}
    		rq->destroy(rq, free);
			pid_t last;
			//I had one too many frees in valgrind
			run->dequeue(run, (void**)&last); 
    		run->destroy(run, free);		
	
		}	
	}
	return 0;
}


