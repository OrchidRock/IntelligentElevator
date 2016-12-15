/*	schedule.h
 *	This is a module shedule elevator base on the each
 *	elevator's shared state and request queue.
 */

#ifndef __CONTROLLER_SHARED_H__
#define __CONTROLLER_SHARED_H__
#include "common.h"

/* Create shared memory with elevator */
typedef struct{
	int elevator_number;
	int *shm_id_array; /* The identifier of shared momory */ 
	sem_t** mutex_array; /* The mutex for each shared memory */
	elevator_shared_t** elevator_shared_array; /* The each shared memory with 
											corresponding elevator */
}multi_shared_t;


int multi_shared_init(multi_shared_t*, int);
void multi_shared_free(multi_shared_t*);
int* multi_shared_get_shmid_array(multi_shared_t*);

//void multi_shared_lock_elevator(multi_shared_t*, int);
int 
multi_shared_get_elevator_shared(multi_shared_t*, elevator_shared_t*,int);
#endif
