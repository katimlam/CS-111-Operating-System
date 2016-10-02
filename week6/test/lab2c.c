#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include "SortedList.h"

int threads = 1;
int iterations = 1;
int syncS = 0;
int syncM = 0;
int opt_yield = 0;
int num_elements = 0;
int num_lists = 1;
pthread_mutex_t* mutexlocks = NULL;
int* locks = NULL;
SortedList_t** shared_list= NULL;
SortedListElement_t** element_set = NULL;
char**  key_set = NULL;

static struct option long_options[] ={
	{ "threads", required_argument, NULL, 't' },
	{ "iterations", required_argument, NULL, 'i' },
	{ "yield", required_argument, NULL, 'y' },
	{ "sync", required_argument, NULL, 's' },
	{ "lists", required_argument, NULL, 'l' }
};
void LOCK(int lock_num)
{
	if (syncM)
		pthread_mutex_lock(&mutexlocks[lock_num]);
	else if (syncS)
		while (__sync_lock_test_and_set(&locks[lock_num], 1))
			;
}
void UNLOCK(int lock_num){
	/*unlock*/
	if (syncM)
		pthread_mutex_unlock(&mutexlocks[lock_num]);
	else if (syncS)
		__sync_lock_release(&locks[lock_num]);
}
int hash_function(const char* key){
	int i;
	int sum = 0;
	for (i = 0; i < 3 && strlen(key); i++)
		sum = sum + (int)key[i];
	return sum % num_lists;
}
void exit_function(void)
{
	if (key_set != NULL)
	{
		int i;
		for (i = 0; i < num_elements; i++)
			if (key_set[i] != NULL)
				free(key_set[i]);
		free((void*)key_set);
	}
	if (element_set != NULL)
		free((void*)element_set);
	if (shared_list != NULL)
		free((void*)shared_list);
	if (mutexlocks != NULL)
		free((void*)mutexlocks);
	if (locks != NULL)
		free((void*)locks);
}
void errorexit(char* error_msg)
{
	perror(error_msg);
	exit(EXIT_FAILURE);
}
void* thread_function(void* thread_arg)
{
	int iter_num = *(int*)thread_arg;
	int i;
	for (i = iter_num; i < num_elements; i = i + threads)
	{
		//printf("thread%d: %s  %s\n", iter_num, element_set[i]->key, key_set[i]);
		int hash_num = hash_function(key_set[i]);
		LOCK(hash_num);
		SortedList_insert(shared_list[hash_num], element_set[i]);
		UNLOCK(hash_num);
	}

	for (i = 0; i < num_lists; i++){
		LOCK(i);
		SortedList_length(shared_list[i]);
		UNLOCK(i);
	}

	for (i = iter_num; i < num_elements; i = i + threads)
	{
		int hash_num = hash_function(key_set[i]);
		/*
		LOCK(hash_num);
		SortedListElement_t* ptr = SortedList_lookup(shared_list[hash_num], element_set[i]->key);
		UNLOCK(hash_num);

		LOCK(hash_num);
		if (SortedList_delete(ptr) == 1){
			UNLOCK(hash_num);
			errorexit("SortedList_delete");
		}
		UNLOCK(hash_num);
		*/
		LOCK(hash_num);
		if (SortedList_delete(SortedList_lookup(shared_list[hash_num], key_set[i])) == 1){
			UNLOCK(hash_num);
			errorexit("SortedList_delete");
		}
		UNLOCK(hash_num);

	}
	return NULL;
}
void str_randomize(char* str, int str_len)
{
	int i;
	for (i = 0; i < str_len; i++)
	{
		str[i] = (rand()%94)+33;
	}
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
		case 'l':
			num_lists = atoi(optarg);
			break;
		case 's':
			if (optarg[0] == 's')
				syncS = 1;
			else if (optarg[0] == 'm')
				syncM = 1;
			else
				errorexit("Invalid argument for --sync");
			break;
		case 'y':
			for (c = 0; c < strlen(optarg); c++){
				switch (optarg[c]){
				case 'i':
					opt_yield |= 0x01;
					break;
				case 'd':
					opt_yield |= 0x02;
					break;
				case 's':
					opt_yield |= 0x04;
					break;
				default:
					errorexit("--yield=[ids]");
				}
			}
			break;
		case ':':
			exit(EXIT_FAILURE);
			break;
		case '?':
			exit(EXIT_FAILURE);
			break;
		}
	}
	if (syncM + syncS > 1)
		errorexit("More than one --Sync option enabled");
	num_elements = threads*iterations;
	atexit(exit_function);

	/*create shared_list and locks*/
	shared_list = calloc(num_lists, sizeof(SortedList_t*));
	for (c = 0; c < num_lists; c++){
		shared_list[c] = malloc(sizeof(SortedList_t));
		shared_list[c]->prev = shared_list[c]->next = shared_list[c];
	}

	mutexlocks = calloc(num_lists, sizeof(pthread_mutex_t));
	for (c = 0; c < num_lists; c++){
		pthread_mutex_init(&mutexlocks[c], NULL);
	}
	locks = calloc(num_lists, sizeof(int));
	for (c = 0; c < num_lists; c++){
		locks[c] = 0;
	}
	
	/*create element_set and key_set*/
	element_set = calloc(num_elements, sizeof(SortedListElement_t*));
	key_set = calloc(num_elements, sizeof(char*));

	/*get random string and put them into element_set*/
	srand(time(0));
	size_t str_len = 5;
	char* random_str = NULL;
	SortedListElement_t* ptr = NULL;
	for (c = 0; c < num_elements; c++){
		ptr = malloc(sizeof(SortedListElement_t));
		random_str = malloc(sizeof(char)*(str_len + 1));
		random_str[str_len] = '\0';
		str_randomize(random_str, str_len);
		ptr->key = random_str;
		element_set[c] = ptr;
		key_set[c] = malloc(sizeof(char)*(str_len + 1));
		strcpy(key_set[c], random_str);
	}
	
	/*for (c = 0; c < threads * iterations; c++){
		printf("%s  %s\n", element_set[c]->key, key_set[c]);
	}*/

	/*thread*/
	pthread_t thread_id[threads];
	int iter_num[threads];
	int t;
	/*get start time*/
	struct timespec start, stop;
	if (clock_gettime(CLOCK_MONOTONIC, &start) == -1) {
		errorexit("ERROR on clock gettime(start)");
	}

	for (t = 0; t < threads; t++)
	{
		iter_num[t] = t;
		if (pthread_create(&thread_id[t], NULL, thread_function, &iter_num[t]) != 0)
			errorexit("ERROR on pthread_create");
	}
	for (t = 0; t < threads; t++)
	{
		if (pthread_join(thread_id[t], NULL) != 0)
			errorexit("ERROR on pthread_join");
	}
	/*
	SortedListElement_t* pp = shared_list->next;
	while (pp != shared_list)
	{
		printf("%d: %s\n", pp->key[0], pp->key);
		pp = pp->next;
	}*/

	/*get stop time*/
	if (clock_gettime(CLOCK_MONOTONIC, &stop) == -1) {
		errorexit("ERROR on clock gettime(stop)");
	}

	long long num_operations = (long long) threads * iterations * 2;
	long long run_time = ((long long)(stop.tv_sec - start.tv_sec)) * 1000000000 + 
		(stop.tv_nsec - start.tv_nsec);
	long long ns_per_op = run_time / num_operations;
	double corrected_per_op = (double)ns_per_op / (double)(num_elements/num_lists);

	int list_len = 0;
	for (c = 0; c < num_lists; c++)
		list_len += SortedList_length(shared_list[c]);
	/*output*/
	dprintf(STDOUT_FILENO, "%d threads x %d iterations x (insert + lookup/delete) = %lld operation\n",
		threads, iterations, num_operations);
	if (list_len != 0)
		dprintf(STDERR_FILENO, "ERROR: final count = %d\n", list_len);
	dprintf(STDOUT_FILENO, "elapsed time: %lldns\n", run_time);
	dprintf(STDOUT_FILENO, "per operation: %lldns\n", ns_per_op);
	dprintf(STDOUT_FILENO, "corrected per operation: %lfns\n", corrected_per_op);
	if (list_len != 0)
		exit(EXIT_FAILURE);
	exit(EXIT_SUCCESS);
}

