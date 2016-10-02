#define _GNU_SOURCE
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <time.h>


int opt_yield = 0;
int threads = 1;
int iterations = 1;
int syncS = 0;
int syncM = 0;
int syncC = 0;
volatile int lock = 0;
pthread_mutex_t mutexsum;
static struct option long_options[] ={
	{ "threads", required_argument, NULL, 't' },
	{ "iterations", required_argument, NULL, 'i' },
	{ "yield", no_argument, &opt_yield, 1 },
	{ "sync", required_argument, NULL, 's' }
};

void add(long long *pointer, long long value) {
	/*lock*/
	if (syncM)
		pthread_mutex_lock(&mutexsum);
	else if (syncS)
		while (__sync_lock_test_and_set(&lock, 1))
			;
	else if (syncC)
		while (__sync_val_compare_and_swap(&lock, 0, 1))
			;
	
	/*critical session*/
	long long sum = *pointer + value;
	if (opt_yield)
		pthread_yield();
	*pointer = sum;
	
	/*unlock*/
	if (syncM) 
		pthread_mutex_unlock(&mutexsum);
	else if (syncS || syncC)
		__sync_lock_release(&lock);
}
void errorexit(char* error_msg)
{
	perror(error_msg);
	exit(EXIT_FAILURE);
}
void* thread_function(void* thread_arg)
{
	for (int i = 0; i < iterations; i++)
	{
		add(thread_arg, 1);
		add(thread_arg, -1);
	}
	return NULL;
}

int main(int argc, char* argv[])
{
	/*parsing arguments.*/
	int c;
	extern char *optarg;
	extern int optind, optopt;
	while ((c = getopt_long(argc, argv, "t:i:s:", long_options, NULL)) != -1)
	{
		switch (c)
		{
		case 't':
			threads = atoi(optarg);
			break;
		case 'i':
			iterations = atoi(optarg);
			break;
		case 's':
			if (optarg[0] == 's')
				syncS = 1;
			else if (optarg[0] == 'm')
				syncM = 1;
			else if (optarg[0] == 'c')
				syncC = 1;
			else
				errorexit("Invalid argument for --sync");
			break;
		case ':':
			exit(EXIT_FAILURE);
			break;
		case '?':
			exit(EXIT_FAILURE);
			break;
		}
	}
	if (syncC + syncM + syncS > 1)
		errorexit("More than one --Sync option enabled");
	
	/*get start time*/
	long long counter = 0;
	struct timespec start, stop;
	if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start) == -1) {
		errorexit("ERROR on clock gettime(start)");
	}
	//printf("start time s:%d\n", start.tv_sec);
	//sprintf("start time ns:%d\n", start.tv_nsec);

	/*thread*/
	pthread_mutex_init(&mutexsum, NULL);
	pthread_t thread_id[threads];
	int t;
	for (t = 0; t < threads; t++)
	{
		if (pthread_create(&thread_id[t], NULL, thread_function, &counter) != 0)
			errorexit("ERROR on pthread_create");
	}
	for (t = 0; t < threads; t++)
	{
		if (pthread_join(thread_id[t], NULL) != 0)
			errorexit("ERROR on pthread_join");
	}

	/*get stop time*/
	if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &stop) == -1) {
		errorexit("ERROR on clock gettime(stop)");
	}
	//printf("stop time s:%d\n", stop.tv_sec);
	//printf("stop time ns:%d\n", stop.tv_nsec);

	long long num_operations = (long long) threads * iterations * 2;
	long long run_time = ((long long)(stop.tv_sec - start.tv_sec)) * 1000000000 + 
		(stop.tv_nsec - start.tv_nsec);
	long long ns_per_op = run_time / num_operations;

	/*output*/
	dprintf(STDOUT_FILENO, "%lld threads x %lld iterations x (add + subtract) = %lld operations\n",
		threads, iterations, num_operations);
	if (counter != 0)
		dprintf(STDERR_FILENO, "ERROR: final count = %d\n", counter);
	dprintf(STDOUT_FILENO, "elapsed time: %lldns\n", run_time);
	dprintf(STDOUT_FILENO, "per operation: %lldns\n", ns_per_op);

	if(syncM)
		pthread_mutex_destroy(&mutexsum);
	if (counter != 0)
		exit(EXIT_FAILURE);
	exit(EXIT_SUCCESS);
}

