#include "elevator_shared.h"

void shared_init(shared_t* my_shared, int id){
	my_shared->mutex = NULL;
	my_shared->elevator_shared = NULL;
	if(id >= 0){
		char* byte_array = (char*)shmat(id, (void*)0, 0);
		if(byte_array == NULL)
			return;
		my_shared->mutex = (sem_t*) byte_array;
		byte_array = byte_array + sizeof(sem_t);
		my_shared->elevator_shared = (elevator_shared_t*)byte_array;
	}
}
void shared_all_init(shared_t* my_shared, int fn){
	if(!shared_isvaild(my_shared)) return;
	sem_wait(my_shared->mutex);
	my_shared->elevator_shared->current_floor_number = fn;
	my_shared->elevator_shared->container_human_number = 0;
	my_shared->elevator_shared->current_state = SLEEPING;
	my_shared->elevator_shared->directive = STOP;
	my_shared->elevator_shared->total_time = 0;
	sem_post(my_shared->mutex);
}
void shared_update_curr_floor(shared_t* my_shared, int new_fn){
	if(!shared_isvaild(my_shared)) return;
	sem_wait(my_shared->mutex);
	my_shared->elevator_shared->current_floor_number = new_fn;
	sem_post(my_shared->mutex);
}
void shared_update_directive(shared_t* my_shared, directive_enum new_dir){
	if(!shared_isvaild(my_shared)) return;
	sem_wait(my_shared->mutex);
	my_shared->elevator_shared->directive = new_dir;
	sem_post(my_shared->mutex);
}
void shared_update_curr_state(shared_t* my_shared, elevator_state_enum new_state){
	if(!shared_isvaild(my_shared)) return;
	sem_wait(my_shared->mutex);
	my_shared->elevator_shared->current_state = new_state;
	sem_post(my_shared->mutex);
}
void shared_update_total_time(shared_t* my_shared, long total_time){
	if(!shared_isvaild(my_shared)) return;
	sem_wait(my_shared->mutex);
	my_shared->elevator_shared->total_time = total_time;
	sem_post(my_shared->mutex);
}
void shared_update_contain_hn(shared_t* my_shared, int contain_hn){
	if(!shared_isvaild(my_shared)) return;
	sem_wait(my_shared->mutex);
	my_shared->elevator_shared->container_human_number = contain_hn;
	sem_post(my_shared->mutex);
}
void shared_update_floor_link(shared_t* my_shared, int* src){
	if(!shared_isvaild(my_shared)) return;
	sem_wait(my_shared->mutex);
	elevator_shared_t* est = my_shared->elevator_shared;
	int n = src[0];
	(est->target_floor_queue)[0] = n;
	for(int i = 1; i < n + 1; i++){
		(est->target_floor_queue)[i] = src[i];
	}
	sem_post(my_shared->mutex);
}
void shared_detach(shared_t* my_shared){
	if(!shared_isvaild(my_shared)) return;
	shmdt((void*)my_shared->mutex);
}
int shared_isvaild(shared_t* my_shared){
	int res = 1;
	if(my_shared->mutex == NULL && my_shared->elevator_shared == NULL){
		res = 0;
	}
	return res;
}
