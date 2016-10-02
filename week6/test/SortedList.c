#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "SortedList.h"

void SortedList_insert(SortedList_t *list, SortedListElement_t *element){

	struct SortedListElement* previous = list;
	struct SortedListElement* next = list->next;

	while (next != list)
	{
		if (strcmp(next->key, element->key)>0)
			break;
		previous = next;
		next = next->next;
	}
	if (opt_yield & INSERT_YIELD)
		pthread_yield();
	element->prev = previous;
	element->next = next;
	previous->next = element;
	next->prev = element;
}
int SortedList_delete(SortedListElement_t *element){
	int ret = 0;
	struct SortedListElement* PREVIOUS = element->prev;
	struct SortedListElement* NEXT = element->next;
	if (PREVIOUS->next != element || NEXT->prev != element)
		ret = 1;
	if (opt_yield & DELETE_YIELD)
		pthread_yield();
	if (ret == 0){
		NEXT->prev = PREVIOUS;
		PREVIOUS->next = NEXT;
		if (element->key!=NULL)
			free((void*) element->key);
		free((void*) element);
	}
	return ret;
}
SortedListElement_t *SortedList_lookup(SortedList_t *list, const char *key){
	SortedListElement_t* ret = NULL;
	struct SortedListElement* ptr = list->next;
	
	while (ptr != list && ptr != NULL)
	{
		if (strcmp(ptr->key, key) == 0)
		{
			if (opt_yield & SEARCH_YIELD)
				pthread_yield();
			ret = ptr;
			break;
		}
		ptr = ptr->next;
	}
	return ret;
}
int SortedList_length(SortedList_t *list){
	int count = 0;
	struct SortedListElement* ptr = list->next;
	
	while (ptr != list && ptr != NULL)
	{
		if (opt_yield & SEARCH_YIELD)
			pthread_yield();
		count += 1;
		ptr = ptr->next;
	}
	if (ptr == NULL)
		count = -1;
	return count;
}
