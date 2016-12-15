#ifndef __ELEVATOR_FLOOR_QUEUE_H__
#define __ELEVATOR_FLOOR_QUEUE_H__
#include "priority_queue.h"

typedef struct{
	priority_queue_t priority_queue;
	int* backup_ptr;	
}floor_queue_t;


/* The interface for caller */
int floor_queue_init(floor_queue_t*);
int floor_queue_insert(floor_queue_t*, int*, int);
void floor_queue_remove(floor_queue_t*); /* Move the curr point to next node */
int floor_queue_getvalue(floor_queue_t*); /* Get the value of curr point to */
void floor_queue_free(floor_queue_t*); /* Free all node */
int floor_queue_next_isnull(floor_queue_t*); /* isnull? next node */
int
floor_queue_get_directive(floor_queue_t*);
void floor_queue_get_floor_queue(floor_queue_t*,int*);
int floor_queue_next_then_isempty(floor_queue_t* my_fq);
int floor_queue_print_queue(floor_queue_t* , char*);
#endif
