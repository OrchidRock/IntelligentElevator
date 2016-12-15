/* 	priority_queue.h
 *	This module implements the genericity 
 *  MT-safe priority queue strcture.
 */

#ifndef __PRIORITY_QUEUE_H__
#define __PRIORITY_QUEUE_H__

#include <semaphore.h>
                                    
typedef struct node{
	void* value;
	struct node* next;
}node_t;


typedef struct{
	node_t* tail;
	node_t* curr;
	sem_t* mutex_ptr;
	int is_support_same;
	int directive;
	int directive_second;
	int first_priority_word_index;
	int second_priority_word_index;
}priority_queue_t;

/* The interface for caller */
int priority_queue_init(priority_queue_t*, int ,int, int, int, int, int);
int priority_queue_insert(priority_queue_t*, void*);
void 
priority_queue_remove(priority_queue_t*, int is_free_value); /* Move the node[index]  */
void* priority_queue_getvalue(priority_queue_t*); /* Get the value of curr point to */
void* priority_queue_next(priority_queue_t*);
void priority_queue_move_tail(priority_queue_t*);
void priority_queue_free(priority_queue_t*, int); /* Free all node */
int priority_queue_is_empty(priority_queue_t*); /* isnull? next node */
int priority_queue_get_value_queue(priority_queue_t*,void**);
int priority_queue_print(priority_queue_t* ,
			  void (*)(void*)) ;
int priority_queue_to_string(priority_queue_t*,char*);
#endif
