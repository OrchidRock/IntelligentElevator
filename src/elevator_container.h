#ifndef __ELEVATOR_CONTAINER_H__
#define __ELEVATOR_CONTAINER_H__
#include <semaphore.h>
#include "common.h"

typedef struct{
	int max_human_number;
	int current_human_number;
	human_t* humans_ptr;
	sem_t mutex;
}container_t;

int container_init(container_t*, int);
void container_free(container_t*);
int container_remove(container_t*,int*, int); /* int : floor number */
int container_add(container_t*, human_t*);
int container_current_human_number(container_t*);
#endif
