#ifndef __ELEVATOR_SHARED_H__
#define __ELEVATOR_SHARED_H__
#include "common.h"

typedef struct{
	sem_t* mutex;
	elevator_shared_t* elevator_shared;	
}shared_t;

void shared_init(shared_t*,int id);
void shared_all_init(shared_t*, int );
void shared_update_curr_floor(shared_t*, int);
void shared_update_directive(shared_t*, directive_enum);
void shared_update_curr_state(shared_t*, elevator_state_enum);
void shared_update_total_time(shared_t*, long);
void shared_update_contain_hn(shared_t*, int);
void shared_update_floor_link(shared_t*, int*);

void shared_detach(shared_t*);
int shared_isvaild(shared_t*);
#endif
