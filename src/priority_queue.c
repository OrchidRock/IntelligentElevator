#include "priority_queue.h"
#include <stdlib.h>
#include <stdio.h>

static int
UP_F(int a, int b){ return a < b;}
static int 
DOWN_F(int a, int b){ return a > b;}
static int
TRUE_F(int a, int b) {return 1;} 
void P(sem_t *) ;
void V(sem_t *);
void init_mutex(sem_t*);

static void 
unix_error(char* msg){
	fprintf(stderr, "%s\n",msg);
	exit(EXIT_FAILURE);
}
inline static void 
get_judhe_pf(int dir, int(*JUDGE_PF)(int,int)){
	if(dir == 0)
		JUDGE_PF = &TRUE_F;
	else if(dir == -1)
		JUDGE_PF = &DOWN_F;
	else
		JUDGE_PF = &UP_F;
}

static int
priority_queue_value_compare(priority_queue_t* my_priority_queue, void* a, void* b, int prio_word_index, int dir){
	static int is_skip = 0;
	int (*JUDGE_PF)(int, int);
	if(dir == 0)
		JUDGE_PF = TRUE_F;
	else if(dir == -1)
		JUDGE_PF = DOWN_F;
	else
		JUDGE_PF = UP_F;

	int a_key_value = *((int*)a + prio_word_index);
	int b_key_value = *((int*)b + prio_word_index);
	int res;
	if(a_key_value == b_key_value){

        //printf("a_key_value: %d  b_key_value: %d dir= %d is_skip= %d\n", a_key_value,b_key_value,dir,is_skip);
		if(is_skip || my_priority_queue->second_priority_word_index < 0)
			res = 0;
		else{
			is_skip = 1;
			res = priority_queue_value_compare(my_priority_queue, a, b, 
							my_priority_queue->second_priority_word_index,
							my_priority_queue->directive_second);
            is_skip = 0;
		}
	}
	else if(JUDGE_PF(a_key_value, b_key_value))
		res = 1;                           
	else
		res = -1;
	return res;
}

int 
priority_queue_init(priority_queue_t* my_priority_queue, int is_mt_safed, 
			int is_support_same, int fpwi, int spwi, int d, int d2){
	if(my_priority_queue == NULL) return -1;
	node_t * tmp_node =(node_t *)malloc(sizeof(node_t));
	if(tmp_node == NULL)	
		return -1; 
	tmp_node->value = NULL; /* Tag this is a sentry node */
	tmp_node->next = NULL;
	my_priority_queue->tail = tmp_node;
	my_priority_queue->curr = tmp_node;	
	if(is_mt_safed){
		if((my_priority_queue->mutex_ptr = (sem_t*)malloc(sizeof(sem_t))) 
						!= NULL){
			init_mutex(my_priority_queue->mutex_ptr);
		}else{
			unix_error("malloc error\n");
		}
	}else
		my_priority_queue->mutex_ptr = NULL;
	my_priority_queue->is_support_same = is_support_same;
	my_priority_queue->first_priority_word_index = fpwi;
	my_priority_queue->second_priority_word_index = spwi;
	my_priority_queue->directive  = d;
	my_priority_queue->directive_second = d2;
	return 0;
}

int 
priority_queue_insert(priority_queue_t* my_priority_queue, void* new_value){
	if(new_value == NULL)
		return 1;

	/* Locked */
	P(my_priority_queue->mutex_ptr);
	node_t* tail_node = my_priority_queue->tail;
	int dir = my_priority_queue->directive;
	int fpwi = my_priority_queue->first_priority_word_index;
	int is_insert = 1;
	int res;
	//printf("new_value = %d dir = %d, fpwi = %d\n", *((int*)new_value),
	//				dir, fpwi);
	while(tail_node->next != NULL){
		res = priority_queue_value_compare(my_priority_queue,
						new_value, tail_node->next->value, fpwi, dir);
		//printf("res = %d\n", res);
		if(res > 0) /* Finded */
			break;
		else if(res == 0){
			if(my_priority_queue->is_support_same) is_insert = 1;
			else is_insert = 0;
			break;
		}
		else{  /* continue finding */
			tail_node = tail_node->next;
		}
	}
	if(is_insert){
		node_t* tmp_node  = (node_t*)malloc(sizeof(node_t));
		if(tmp_node == NULL){
			V(my_priority_queue->mutex_ptr);
			return 1;
		}
		tmp_node->value = new_value;
		tmp_node->next = tail_node->next;
		tail_node->next = tmp_node;
	    //tail_node = tail_node->next;
	}
	/* Unlocked*/
	V(my_priority_queue->mutex_ptr);
	return 0;
}

void 
priority_queue_remove(priority_queue_t* my_priority_queue,
				int is_free_value){
	P(my_priority_queue->mutex_ptr);
	node_t * tmp_node = my_priority_queue->curr;
	if(tmp_node->next != NULL){
		node_t * t = tmp_node->next;
		my_priority_queue->curr->next = t->next;
		if(is_free_value) free(t->value);
		free(t);
	}
	V(my_priority_queue->mutex_ptr);
}

void*
priority_queue_getvalue(priority_queue_t* my_priority_queue){
	void* res_value;
	P(my_priority_queue->mutex_ptr);
	if(my_priority_queue->curr->next == NULL)
		res_value = NULL;
	else
		res_value = my_priority_queue->curr->next->value;
	V(my_priority_queue->mutex_ptr);
	return res_value;
}

void*
priority_queue_next(priority_queue_t* my_priority_queue){
	void* res_value = NULL;
	P(my_priority_queue->mutex_ptr);
	node_t * tmp_node;
	if(my_priority_queue->curr != NULL){
		tmp_node = my_priority_queue->curr->next;
		if(tmp_node == NULL)
			res_value = NULL;
		else
			res_value = tmp_node->value;
		my_priority_queue->curr = tmp_node;
	}
	V(my_priority_queue->mutex_ptr);
	return res_value;
}
void
priority_queue_move_tail(priority_queue_t* my_priority_queue){
	P(my_priority_queue->mutex_ptr);
	my_priority_queue->curr = my_priority_queue->tail;
	V(my_priority_queue->mutex_ptr);
}
void 
priority_queue_free(priority_queue_t* my_priority_queue, int is_free_value){
	if(my_priority_queue == NULL) return;
	node_t * tmp_node = my_priority_queue->tail->next;
	while(tmp_node != NULL){
		node_t* tmp_node2 = tmp_node->next;
		if(is_free_value) free(tmp_node->value);
		free(tmp_node);
		tmp_node = tmp_node2;
	}
	if(my_priority_queue->mutex_ptr != NULL)
		free(my_priority_queue->mutex_ptr);
	free(my_priority_queue->tail);
}
int 
priority_queue_is_empty(priority_queue_t* my_priority_queue){
	int res = 0;
	P(my_priority_queue->mutex_ptr);
	if(my_priority_queue->tail->next == NULL)
		res = 1;
	else
		res = 0;
	V(my_priority_queue->mutex_ptr);
	return res;
}

int
priority_queue_get_value_queue(priority_queue_t* my_priority_queue, void** dest){
	int count = 0;
	P(my_priority_queue->mutex_ptr);
	node_t* tmp_node = my_priority_queue->tail->next;
	while(tmp_node != NULL){
		dest[count++] = tmp_node->value;
    	tmp_node = tmp_node->next;
    }                                        
	V(my_priority_queue->mutex_ptr);
	return count;
}
int
priority_queue_print(priority_queue_t* my_priority_queue,
			 void (*callback)(void* value)){
	int count = 0;
	P(my_priority_queue->mutex_ptr);
	node_t* tmp_node = my_priority_queue->tail->next;
	while(tmp_node != NULL){
		callback(tmp_node->value);
    	tmp_node = tmp_node->next;
		count++;
    }                                        
	V(my_priority_queue->mutex_ptr);                   
	return count;	
}
int
priority_queue_to_string(priority_queue_t* my_priority_queue, char* buf){
	int count = 0;
	P(my_priority_queue->mutex_ptr);
	node_t* tmp_node = my_priority_queue->tail->next;
	while(tmp_node != NULL){
		sprintf(buf,"%s%d ",buf,*((int*)tmp_node->value));
    	tmp_node = tmp_node->next;
		count++;
    }                                        
	V(my_priority_queue->mutex_ptr);  
	return count;
}
inline void 
init_mutex(sem_t* sem){
	if(sem != NULL && sem_init(sem, 0, 1) < 0)
		unix_error("sem_init error");
}
inline void 
P(sem_t* sem){
	if(sem != NULL && sem_wait(sem) < 0)
		unix_error("sem_wait error");
}
inline void 
V(sem_t * sem){
	if(sem != NULL && sem_post(sem) < 0)
		unix_error("sem_post error");
}
