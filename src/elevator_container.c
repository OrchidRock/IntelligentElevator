#include "elevator_container.h"
#include "common.h"

int 
container_init(container_t* my_container, int max){
	my_container->max_human_number = max;
	my_container->current_human_number = 0;
	human_t* tmp = (human_t*) malloc(
					sizeof(human_t) * max);
	if(tmp == NULL ){
		my_container->max_human_number = -1;
		my_container->humans_ptr = NULL;
		return -1;  /* Error occured */
	}
	my_container->humans_ptr = tmp;
	for(int i = 0; i < max ; i++){
		tmp[i].start_time = -1;
		tmp[i].target_floor  = -1;
	}
	sem_init(&my_container->mutex, 0, 1);
	return 0;
}

void 
container_free(container_t* my_container){
	sem_wait(&my_container->mutex);
	if(my_container->humans_ptr != NULL){
		free(my_container->humans_ptr);
	}
	sem_post(&my_container->mutex);
}

int 
container_remove(container_t* my_container, 
				int* time_array, int fn){
	if(fn < 0 || time_array == NULL)
		return -1; /* Invail data*/
	sem_wait(&my_container->mutex);
	human_t* tmp = my_container->humans_ptr;
	int count = 0;
	for(int i = 0; i < my_container->max_human_number;
					i++){
		if(tmp[i].target_floor == fn){
			time_array[count++] = tmp[i].start_time;
			tmp[i].target_floor = -1;
			tmp[i].start_time = -1;
			(my_container->current_human_number)--;
		}
	}
	//count = my_container->current_human_number;
	sem_post(&my_container->mutex);
	return count;
}

int 
container_add(container_t* my_container, human_t* h){	
	int res = -1;
	if(h == NULL )
		return res;
	sem_wait(&my_container->mutex);
	human_t* tmp = my_container->humans_ptr;
	for(int i = 0; i < my_container->max_human_number;
					i++){
		if(tmp[i].target_floor < 0 
			&& tmp[i].start_time < 0){ /* Find a slot*/
			res = 0;
			tmp[i].target_floor = h->target_floor;
			tmp[i].start_time = h->start_time;
			(my_container->current_human_number) ++;
			res = my_container->current_human_number;
			break;
		}
	}
	sem_post(&my_container->mutex);
	return res;
}
int container_current_human_number(container_t* my_container){
	int res = 0;
	sem_wait(&my_container->mutex);
	res = my_container->current_human_number;
	sem_post(&my_container->mutex);
	return res;
}
