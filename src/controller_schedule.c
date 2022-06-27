#include "controller_schedule.h"
#define EMN 6 /*ELEVATOR_MAX_NUMBER */

/* Global variables */
int elevator_number;
int max_human_each_elevator;
int elevator_max_floor_array[EMN];
elevator_shared_t elevator_shared_array[EMN];
//static request_t try_go_one_floor_req = {-1,1,-1};

/* function declarations */
int 
on_duty_schedule(schedule_t*, request_t*, int*);
int 
off_duty_schedule(schedule_t*, request_t*, int*);
int 
normal_schedule(schedule_t*, request_t*, int*);
int
in_passing_principle(request_t*, int, elevator_shared_t*,int);



inline int
in_passing_principle(request_t* req_ptr, int dir, elevator_shared_t* 
				my_shared, int predecessor_count){
	int res = -1;
	int sfn = req_ptr->source_floor;
	int tfn = req_ptr->target_floor;
	//printf("----in_passing_principle %d %d ---\n", sfn, tfn);
	if(dir == my_shared->directive ){
		int* queue = my_shared->target_floor_queue;
		for(int i = 1 ; i < queue[0] ; i++){
			if( (queue[i] == sfn || queue[i] == tfn)
				&& (my_shared->container_human_number + 
						predecessor_count)
				   <= (max_human_each_elevator * 0.5)){
				if(queue[i] == tfn && ((queue[0] - sfn)*dir)>0)
					break;   /* The sfn out of bound of queue */
				else{
					res = 0;
					break;
				}
			}
		}
	}
	return res;
}

int
schedule_init(schedule_t* my_schedule, multi_shared_t* mst,
				schedule_strategy_enum sse, 
				int en,int mfn, int max_human_each_ele){
	my_schedule->my_mst = mst;
	my_schedule->my_sse = sse;
	//my_schedule->my_callback = callback;
	/* */
	if(sse == ON_DUTY && en <= EMN){
		float factor_elevator = 0.5;
		float factor_fn = 0.75;
		int m = (int)(en * factor_elevator);
		int i;
		for(i = 0; i < m ; i++){
			elevator_max_floor_array[i] = (int)(mfn * factor_fn);
		}
		for(; i < en ; i++)
			elevator_max_floor_array[i]  = mfn;
	}
	max_human_each_elevator = max_human_each_ele;
	elevator_number = en;
	return 0;
}

int
schedule(schedule_t* my_schedule, request_t* req_ptr, int * predecessor_array){
	int res = -1 , i;

	/* Get the new shared state of all elevator */
	for(i = 0; i < elevator_number; i++){
		multi_shared_get_elevator_shared(my_schedule->my_mst,
						&elevator_shared_array[i], i);
		/*LOCK*/
	//	multi_shared_lock_elevator(my_schedule->my_mst, i);
	}

	/*LOCK*/

	schedule_strategy_enum sse = my_schedule->my_sse;
	if(sse == ON_DUTY){
		res = on_duty_schedule(my_schedule, req_ptr, predecessor_array);
	}else if(sse == OFF_DUTY){
		res = off_duty_schedule(my_schedule, req_ptr, predecessor_array);
	}else{
		res = normal_schedule(my_schedule, req_ptr, predecessor_array);
	}
		
	return res;
}
inline int
on_duty_schedule(schedule_t* my_schedule, request_t* req_ptr,
				int* predecessor_array){
	int res = -1, i = 0, dir;
	int sfn = req_ptr->source_floor;
	dir = (sfn < req_ptr->target_floor) ? 1 : -1;

	for(i = 0; i < elevator_number; i++){
		/*if(elevator_shared_array[i].current_state == SLEEPING)
			my_schedule->my_callback(&try_go_one_floor_req, i, 1);*/
		//printf("elevetor %d dir = %d\n", i,elevator_shared_array[i].directive);
		elevator_state_enum tmp_ese =                            	
        				elevator_shared_array[i].current_state;		

		if( (req_ptr->target_floor <= elevator_max_floor_array[i])
			&& tmp_ese != RUNNING
			&& elevator_shared_array[i].current_floor_number == sfn
			&& (elevator_shared_array[i].container_human_number +
				 predecessor_array[i])<
				max_human_each_elevator
			&& ((tmp_ese == ARRIVED
				&& (elevator_shared_array[i].target_floor_queue[0]<=0
				|| elevator_shared_array[i].directive == dir)
				)
				|| tmp_ese == SLEEPING)){
			res = 1;
			req_ptr->is_preprocessing = i;
			//my_schedule->my_callback(req_ptr, i, 0);
			break;
		}
	}                                                                
	if(i == elevator_number) { /* Not finding*/
		if(req_ptr->is_preprocessing)
			res = -1;
		else if(sfn == 1){ /* First level*/
			/* Add some important code wakeup all sleeping elevator */
			for(int j = 0; j < elevator_number; j++){
				elevator_state_enum tmp_ese = 
							elevator_shared_array[j].current_state;		
				if(tmp_ese != RUNNING){
					if(elevator_shared_array[j].container_human_number +
                      	predecessor_array[j] >= max_human_each_elevator)
						continue;
					if(tmp_ese != RUNNING){
						if((tmp_ese == ARRIVED &&
							(elevator_shared_array[j].target_floor_queue)[0]<=0)
							|| tmp_ese == SLEEPING){
							req_ptr->is_preprocessing = j;
							res = 0;
							//my_schedule->my_callback(req_ptr, j, 1);
							break;
						}
					}
				}
			}                                                              
		}else if(dir == 1){ /* Second level*/
			for(int j = 0 ; j < elevator_number ; j++){
				//elevator_state_enum tmp_ese = 
						//elevator_shared_array[j].current_state;
				if(elevator_shared_array[j].container_human_number +
                   	predecessor_array[j] >= max_human_each_elevator)
					continue;
				if(	elevator_shared_array[j].current_floor_number != 1
					&& req_ptr->target_floor <= elevator_max_floor_array[j]
					&& (in_passing_principle(req_ptr, dir,
							&(elevator_shared_array[j]),
							predecessor_array[j]) >= 0
				    || (elevator_shared_array[j].current_state == SLEEPING))){
					req_ptr->is_preprocessing = j;
					res = 0;
					//my_schedule->my_callback(req_ptr, j , 1);
					break;
				}
			}                                                                
		}else{  /* Third level*/
			for(int j = 0 ; j < elevator_number ; j++){      
				if(elevator_shared_array[j].container_human_number +
                   	predecessor_array[j] >= max_human_each_elevator)
					 continue;	
        		if(req_ptr->source_floor <= elevator_max_floor_array[j]
					&& ((dir == elevator_shared_array[j].directive
						&& req_ptr->target_floor == 1
						) || (elevator_shared_array[j].current_floor_number != 1
							  && elevator_shared_array[j].current_state == SLEEPING)
						)){
					//printf("current state:%d\n",
					//				elevator_shared_array[j].current_state);
        			req_ptr->is_preprocessing = j;
        			res = 0;
					//my_schedule->my_callback(req_ptr, j , 1);
        			break;
        		}
        	}                                                                
		}
	}                                            
	return res;                                                      
}
inline int
off_duty_schedule(schedule_t* my_schedule, request_t * req_ptr,
				int* predecessor_array){
	int res = -1, i = 0, dir;
	int sfn = req_ptr->source_floor;
    dir = (sfn < req_ptr->target_floor) ? 1 : -1;

	for(i = 0; i < elevator_number; i++){
		if(	elevator_shared_array[i].current_state != RUNNING
			&& elevator_shared_array[i].current_floor_number == sfn
			&& elevator_shared_array[i].container_human_number <
				max_human_each_elevator
			&& (elevator_shared_array[i].directive == STOP 
				|| elevator_shared_array[i].directive == dir)){
		 	res = 1;
			req_ptr->is_preprocessing = i;
			break;
		}
	}                                                                
	if( i == elevator_number){
		if(req_ptr->is_preprocessing)
			res = -1;
		else if(req_ptr->target_floor == 1){
			for(int j = 0; j < elevator_number; j++){                       	
        		elevator_state_enum tmp_ese = 
        					elevator_shared_array[j].current_state;		
        		if(tmp_ese != RUNNING           /* ???????????????? replace '!= RUNNING '*/
					&& (elevator_shared_array[j].directive == STOP
						|| elevator_shared_array[j].directive == dir)){
        			req_ptr->is_preprocessing = j;
        			res = 0;
       				break;
       			}
       		}                                                          
		}else if(dir == 1){ /* Second UP */
			for(int j = 0 ; j < elevator_number ; j++){                      
        		if(elevator_shared_array[j].current_state == SLEEPING
        			|| (dir == elevator_shared_array[j].directive
						&& elevator_shared_array[j].container_human_number
							< max_human_each_elevator	
        				&& req_ptr->source_floor == 1)){
        			req_ptr->is_preprocessing = j;
        			res = 0;
        			break;
        		}
        	}                                                                                                       
		}else{ /* DOWN */
			for(int j = 0 ; j < elevator_number ; j++){                    		
        		if(elevator_shared_array[j].current_state == SLEEPING
        			|| (dir == elevator_shared_array[j].directive
						&& elevator_shared_array[j].container_human_number
						  < max_human_each_elevator )){
        			req_ptr->is_preprocessing = j;
        			res = 0;
        			break;
        		}
        	}                                                              
		}
	}
	return res;
}
inline int
normal_schedule(schedule_t* my_schedule, request_t* req_ptr,
				int* predecessor_array){
	int res = -1, i = 0, dir;
	int sfn = req_ptr->source_floor;
    dir = (sfn < req_ptr->target_floor) ? 1 : -1;
	for(i = 0; i < elevator_number; i++){
        if(i==1){
            /*printf("elevator 1: %d %d %d %d\n",elevator_shared_array[1].current_state,
                                               elevator_shared_array[1].current_floor_number,
                                               elevator_shared_array[1].container_human_number,
                                               elevator_shared_array[1].directive);*/
        }
		if(	elevator_shared_array[i].current_state != RUNNING
			&& elevator_shared_array[i].current_floor_number == sfn
			&& elevator_shared_array[i].container_human_number <
				max_human_each_elevator
			&& (elevator_shared_array[i].directive == STOP 
				|| elevator_shared_array[i].directive == dir)){
		 	res = 1;
			req_ptr->is_preprocessing = i;
			break;
		}
	}                                                                
	if( i == elevator_number){
		if(req_ptr->is_preprocessing)
			res = -2;
		else{
			int j;
			for(j = 0; j < elevator_number ; j++){
				if(elevator_shared_array[j].current_state == SLEEPING && 
                    predecessor_array[j] < EMN){
					res = 0;
					req_ptr->is_preprocessing = j;
					break;
				}
			}
			if(j == elevator_number){
				for(int k = 0 ; k < elevator_number ; k++){
					if(elevator_shared_array[k].directive == dir
						&& elevator_shared_array[k].container_human_number
								< max_human_each_elevator
                        && predecessor_array[k] < EMN 
                        && ((dir == 1 && elevator_shared_array[k].current_floor_number <= sfn) ||
                            (dir == -1 && elevator_shared_array[k].current_floor_number >= sfn))){
						res = 0;
						req_ptr->is_preprocessing = k;
						break;
					}
				}
			}
		}
	}
    /*printf("Normal_Schedule: %d %d %d %d --- ", req_ptr->start_time, 

                                             req_ptr->source_floor,
                                             req_ptr->target_floor,
                                             req_ptr->is_preprocessing);
    printf("res = %d\n", res);*/
	return res;
}
