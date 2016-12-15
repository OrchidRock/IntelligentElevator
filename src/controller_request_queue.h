#ifndef __CONTROLLER_REQUEST_QUEUE_H__
#define __CONTROLLER_REQUEST_QUEUE_H__
#include "priority_queue.h"
#include "controller_schedule.h"
#include "common.h"
typedef struct{
	priority_queue_t* first_level_rq;
	priority_queue_t* second_level_rq;
	priority_queue_t* third_level_rq;
}multi_request_queue_t;

void 
multi_request_queue_init(multi_request_queue_t*, schedule_strategy_enum);
int
multi_request_queue_insert(multi_request_queue_t*, int level, request_t*);

int multi_request_queue_is_empty(multi_request_queue_t*, int level);
priority_queue_t * 
multi_request_queue_get_pq(multi_request_queue_t*,int);
void 
multi_request_queue_free(multi_request_queue_t*);
#endif
