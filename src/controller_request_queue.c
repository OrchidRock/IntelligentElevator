#include "controller_request_queue.h"
priority_queue_t*
get_pqt_by_level(multi_request_queue_t*, int);

inline priority_queue_t*
get_pqt_by_level(multi_request_queue_t* my_mrq, int level){
	if(level == 0)
		return my_mrq->first_level_rq;
	else if(level == 1 )
		return my_mrq->second_level_rq;
	else
		return my_mrq->third_level_rq;
}


/*
 *	level = 0 ====> first_level_rq
 *			1 ====> second_level_rq
 *			2 ====> third_level_rq
 */
void 
multi_request_queue_init(multi_request_queue_t* my_mrq,
				schedule_strategy_enum ss){
	my_mrq->second_level_rq = (priority_queue_t*)malloc(
						sizeof(priority_queue_t));
	my_mrq->third_level_rq = (priority_queue_t*)malloc(
				sizeof(priority_queue_t));            
	if(ss == ON_DUTY){
	
		my_mrq->first_level_rq = (priority_queue_t*)malloc(
					sizeof(priority_queue_t));         		
		/* UP from 1 floor*/
		priority_queue_init(my_mrq->first_level_rq, 1, 1,0, -1, 1, -1);
		/* UP from other floor */
		priority_queue_init(my_mrq->second_level_rq, 1, 1,1, 0, 1, 1);
		/* DOWN */
		priority_queue_init(my_mrq->third_level_rq, 1,1,2, 0, 1, 1); 
	}else if(ss == OFF_DUTY){
		my_mrq->first_level_rq = (priority_queue_t*)malloc(
					sizeof(priority_queue_t));         		
		/* DOWN to 1 floor */
		priority_queue_init(my_mrq->first_level_rq, 1, 1,1, 0, -1, 1);
		/* UP */
		priority_queue_init(my_mrq->second_level_rq, 1, 1,2, 0, -1, 1);
		/* DOWN to other floor */
		priority_queue_init(my_mrq->third_level_rq, 1, 1,2, 0, -1, 1); 
	}else{ /* NORMAL */
		/* UP */
		priority_queue_init(my_mrq->second_level_rq, 1, 1,0, -1, 1, -1);
		/* DOWN */
		priority_queue_init(my_mrq->third_level_rq, 1, 1,0, -1, 1, -1);
		my_mrq->first_level_rq = NULL;
	}
}
int
multi_request_queue_insert(multi_request_queue_t* my_mrq,
						int level, request_t* req){
	int res;
	priority_queue_t* tmp_level_rq = get_pqt_by_level(my_mrq, level);
	res = priority_queue_insert(tmp_level_rq, (void*)req);
	return res;
}
int
multi_request_queue_is_empty(multi_request_queue_t* my_mrq, int level){
	int res;
	priority_queue_t* tmp_level_rq = get_pqt_by_level(my_mrq, level);
	res = priority_queue_is_empty(tmp_level_rq);
	return res;
}
priority_queue_t*
multi_request_queue_get_pq(multi_request_queue_t* my_mrq, int level){
	return get_pqt_by_level(my_mrq, level); 
	//return (priority_queue_t*)(my_mrq + level);
}
void
multi_request_queue_free(multi_request_queue_t* my_mrq){
	priority_queue_free(my_mrq->first_level_rq, 0);
	priority_queue_free(my_mrq->second_level_rq, 0);
	priority_queue_free(my_mrq->third_level_rq, 0);
	if(my_mrq->first_level_rq != NULL)
		free(my_mrq->first_level_rq);
	if(my_mrq->second_level_rq != NULL)
		free(my_mrq->second_level_rq);
	if(my_mrq->third_level_rq != NULL)
		free(my_mrq->third_level_rq);

}
