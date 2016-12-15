#include "controller_shared.h"
int multi_shared_init(multi_shared_t* my_shared, int ele_max_numble){
	my_shared->mutex_array = (sem_t**) malloc(sizeof(sem_t*) * ele_max_numble);
	if(my_shared->mutex_array == NULL){
		return -1;		
	}
	my_shared->elevator_shared_array = (elevator_shared_t**)malloc(
					sizeof(elevator_shared_t*) * ele_max_numble);
	if(my_shared->elevator_shared_array == NULL)
		return -1;
	my_shared->shm_id_array = (int*)malloc(sizeof(int) * ele_max_numble);
	if(my_shared->shm_id_array == NULL)
		return -1;
	for(int i= 0 ; i < ele_max_numble;i++){
		my_shared->shm_id_array[i] = shmget((key_t)(1234+i), sizeof(sem_t) + 
						sizeof(elevator_shared_t), 0644|IPC_CREAT);
		if(my_shared->shm_id_array[i] < 0){
			//shared_free(ele_max_numble);
			return -1;
		}
	}
	for(int i = 0; i < ele_max_numble ; i++){
		char* byte_array = (char*)shmat(my_shared->shm_id_array[i], (void*)0, 0);
		if(byte_array == NULL){
			//shared_free(ele_max_numble);
			return -1;
		}
		my_shared->mutex_array[i] = (sem_t*)byte_array;
		byte_array = byte_array + sizeof(sem_t);
		sem_init(my_shared->mutex_array[i], 0 , 1);

		my_shared->elevator_shared_array[i] = (elevator_shared_t*)byte_array;
		((my_shared->elevator_shared_array[i])->target_floor_queue)[0] = 0;
	}
	my_shared->elevator_number = ele_max_numble;
	return 0;
}
void multi_shared_free(multi_shared_t* my_shared){
	int ele_max_numble = my_shared->elevator_number;
	for(int i = 0; i < ele_max_numble; i++){
		shmdt((void*)my_shared->mutex_array[i]);
		shmctl(my_shared->shm_id_array[i], IPC_RMID, 0);
	}
	free(my_shared->mutex_array);
	free(my_shared->elevator_shared_array);
	free(my_shared->shm_id_array);
}
int* multi_shared_get_shmid_array(multi_shared_t* my_shared){
	return my_shared-> shm_id_array;
}
/*
   void multi_shared_lock_elevator(multi_shared_t* my_shared, int en){
	sem_wait((my_shared->mutex_array)[en]);
}
*/
int multi_shared_get_elevator_shared(multi_shared_t* my_shared,
							elevator_shared_t* dest, int en){
	if(en >= my_shared->elevator_number)
		return -1;
	elevator_shared_t* src = (my_shared->elevator_shared_array)[en];
	
	sem_wait((my_shared->mutex_array)[en]);
	dest->container_human_number = src->container_human_number;
	dest->current_state = src->current_state;
	dest->directive = src->directive;
	dest->current_floor_number = src->current_floor_number;
	int n = (src->target_floor_queue)[0];
	(dest->target_floor_queue)[0] = n;
	for(int i = 1; i < n + 1 ; i++){
		(dest->target_floor_queue)[i] = (src->target_floor_queue)[i];
	}
	dest->total_time = src->total_time;
	sem_post((my_shared->mutex_array)[en]);
	return 0;
}
