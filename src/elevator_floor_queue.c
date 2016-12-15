#include "elevator_floor_queue.h"
#include "common.h"
static int* backup_ptr;

static void floor_queue_value_translate_callback(void*);
int floor_queue_init(floor_queue_t* my_fq){
	int res = 0;
	my_fq->backup_ptr = NULL;
	res = priority_queue_init(&(my_fq->priority_queue), 1, 0, 0, -1, 0, -1);
	return res;
}
int floor_queue_insert(floor_queue_t* my_fq, int* new_values,
				int dir){
	int n = new_values[0];
	int res = 0;
	if(dir >=-1 && dir <= 1)
		(my_fq->priority_queue).directive = dir;
	int * value_ptr;
	for(int i = 1 ; i < n + 1; i++){
		value_ptr = (int*)malloc(sizeof(int));
		*value_ptr = new_values[i];
		//printf("pid: %d ---fqi--: %d \n", getpid(),new_values[i]);
		res = priority_queue_insert(&(my_fq->priority_queue),
						(void*)value_ptr);
	}
	return res;
}
int
floor_queue_get_directive(floor_queue_t* my_fq){
	return (my_fq->priority_queue).directive;
}
int floor_queue_next_then_isempty(floor_queue_t* my_fq){
	priority_queue_t* tmp = &(my_fq->priority_queue);
	priority_queue_next(tmp);
	return priority_queue_is_empty(tmp);
}
void floor_queue_remove(floor_queue_t* my_fq){
	priority_queue_t* tmp = &(my_fq->priority_queue);
	priority_queue_remove(tmp, 1);
}
int floor_queue_getvalue(floor_queue_t* my_fq){
	void* res =  priority_queue_getvalue(&(my_fq->priority_queue));
	if(res == NULL)
		return -1;
	else
		return *((int*)res);
}
void floor_queue_free(floor_queue_t* my_fq){
	priority_queue_free(&(my_fq->priority_queue), 1);
}
/* Depatch */
int floor_queue_next_isnull(floor_queue_t* my_fq){
	return priority_queue_is_empty(&(my_fq->priority_queue));
}
void floor_queue_get_floor_queue(floor_queue_t* my_fq, int* dest){
	int ** src = (int**)malloc(sizeof(int*) * MFN);
	int count = priority_queue_get_value_queue(&(my_fq->priority_queue),
						(void**)src);
	count = count % MFN;
	dest[0]	 = count;
	for(int i = 1 ; i < count + 1; i++){
		dest[i] = *((int*)src[i-1]);
	}
	free(src);
}
static void floor_queue_value_translate_callback(void* value){
	if(backup_ptr != NULL){
		*backup_ptr = *((int*)value);
		backup_ptr ++;
	}
}
int floor_queue_print_queue(floor_queue_t* my_fq, char* buf){
	return priority_queue_to_string(&(my_fq->priority_queue),buf);
}
