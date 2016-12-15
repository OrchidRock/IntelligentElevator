#ifndef __CONTROLLER_SCHEDULE_H__
#define __CONTROLLER_SCHEDULE_H__
#include "controller_shared.h"
typedef enum{
	ON_DUTY = 0, /* There are a lot of requests at 1 floor 
				  is on duty */
	OFF_DUTY, /* There are a lot of requests at high floor
				will go to 1 floor is off duty */
	NORMAL
}schedule_strategy_enum;
//typedef
typedef struct{
	multi_shared_t * my_mst;
	//int elevator_number;
	schedule_strategy_enum my_sse;
//	void (*my_callback)(request_t*,int,int);
}schedule_t;

int
schedule_init(schedule_t*, multi_shared_t*, schedule_strategy_enum,
			int,int,int);
int
schedule(schedule_t*, request_t*,int*);
#endif
