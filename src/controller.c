/*	controller.c
 *	Describe:
 *		This moudle as the controller for n-elevators.
 *		This controller try to improve the bandwidth of
 *		running by it's schedule algorithm.
 *	Date:
 *		2016-10-24
 *	Author:
 *		Rock. cqugyl@163.com
 */

#include "common.h"
#include "controller_shared.h"
#include "controller_schedule.h"
#include "controller_request_queue.h"
/* The conf macro */
#define MAX_FLOOR_NUMBER 20
#define ELEVATOR_NUMBER 4
#define MAX_HUMAN_EACH_ELE 6
#define RUN_ONE_FLOOR_TIME 3
#define STOP_ONE_FLOOR_TIME 10
#define DATASET_MAXLINE 151
typedef struct{
	request_t* req_ptr;
}request_ptr_t;       


/* The functions prototype */
void 
sigint_handler(int sig);
void sigalrm_handler(int sig){}
void print_elevator_state(int);
void * thread_handler(void*);
void sigusr2_handler(int sig){}
void insert_queue(request_t*, int);
void print_request_queue();
void print_dynamic_state_table();
void care_multi_request_queue(int);
void 
send_human_to_elevator(int*, request_ptr_t[][MAX_HUMAN_EACH_ELE]);

/* Global variables */
int pm[ELEVATOR_NUMBER][2]; /* pipe*/
multi_shared_t my_multi_shared;
multi_request_queue_t my_multi_rq;
schedule_t my_schedule;
int is_accept_intime_reauest;/* */
schedule_strategy_enum my_sche_strategy; /* */
sem_t local_mutex;
volatile int current_time;
int 
main(int argc, char* argv[]){
	int pid[ELEVATOR_NUMBER + 1];
	int i;
	//int startup_time;
	//int current_time;
	request_t dataset[DATASET_MAXLINE];
	int human_count = 0 ;
	int	dataset_count = 0;
	int dataset_index  = -1;
	char buf[BUFSIZE];
	pthread_t thread;

	/* Input data set */
	FILE *fd_stream = NULL;
	if(argc > 1){
		fd_stream = fopen(argv[1] , "r");
		if(fd_stream != NULL){
			char * delim;
			char * tmp_ptr;
			while(fgets(buf, BUFSIZE, fd_stream) != NULL){
				buf[strlen(buf)-1] = '\0';
				//printf("buf: %s\n", buf);
				tmp_ptr = buf;
				if((delim = strstr(tmp_ptr, " ")) != NULL){
					*delim = '\0';
					dataset[dataset_count].start_time = atoi(tmp_ptr);
					tmp_ptr = delim + 1;
				} 
				if((delim = strstr(tmp_ptr, " ")) != NULL){         	
            		*delim = '\0';
            		dataset[dataset_count].source_floor = atoi(tmp_ptr);
            		tmp_ptr = delim + 1;
            	}                                              
            	dataset[dataset_count].target_floor = atoi(tmp_ptr);
				dataset[dataset_count].is_preprocessing = 0;	
				dataset_count = (dataset_count + 1) % DATASET_MAXLINE;
			}
			is_accept_intime_reauest = 0;
			dataset_index = 0;
			fclose(fd_stream);
		}
	}else{
		fprintf(stderr,"Warning: The dataset file argument hasn't set!");
	}
	my_sche_strategy = NORMAL;
	if(argc > 2){ /* The schedule_strategy */
		if(strcmp(argv[2], "-on") == 0 ) /* ON_DUTY */
			my_sche_strategy = ON_DUTY;
		else if(strcmp(argv[2], "-off") == 0) /* OFF_DUTY */
			my_sche_strategy = OFF_DUTY;
		else if(strcmp(argv[2], "-normal") == 0)
			my_sche_strategy = NORMAL;
		else
			fprintf(stderr, "%s invaild argument\n", argv[2]);
	}

	//char buf[BUFSIZE];
	char args[7][15];

	/* Insatll signal handler */
	if(signal(SIGINT, sigint_handler) < 0){
		fprintf(stderr, "signal error %s\n",
							strerror(errno));
	}
	if(signal(SIGALRM, sigalrm_handler) < 0){
		fprintf(stderr, "signal error %s\n",
							strerror(errno));
		exit(EXIT_FAILURE);	
	}                                                                       
	if(signal(SIGUSR2, sigusr2_handler) < 0){
		fprintf(stderr, "signal error %s\n",
							strerror(errno));
	}              

	/* Create the pipes */
	for(i = 0; i < ELEVATOR_NUMBER ; i++){
		if(pipe(pm[i]) < 0){
			fprintf(stderr, "pipe error %s\n",
							strerror(errno));
			exit(EXIT_FAILURE);
		}
	}                                       
	/* Init Schedule  and request queue */
	if(multi_shared_init(&my_multi_shared, ELEVATOR_NUMBER) < 0){
			fprintf(stderr, "multi_shared_init error\n");
			exit(EXIT_FAILURE);
	}
	multi_request_queue_init(&my_multi_rq, my_sche_strategy);
	if(schedule_init(&my_schedule, &my_multi_shared, my_sche_strategy,
		ELEVATOR_NUMBER, MAX_FLOOR_NUMBER, MAX_HUMAN_EACH_ELE) < 0)	{
		fprintf(stderr, "schedule_init error\n");
	}

	sem_init(&local_mutex, 0, 1);
	/* Create the child process */
		/* Init the arguments for child process */
	sprintf(args[1],"%d", RUN_ONE_FLOOR_TIME);
	sprintf(args[2],"%d", STOP_ONE_FLOOR_TIME);
	sprintf(args[3],"%d", MAX_FLOOR_NUMBER);
	sprintf(args[4],"%d", MAX_HUMAN_EACH_ELE);
	int * shm_ids = multi_shared_get_shmid_array(&my_multi_shared);

	for(i = 0 ; i < ELEVATOR_NUMBER ; i++){
		if((pid[i] = fork()) < 0){
			fprintf(stderr, "fork error %s\n",
						strerror(errno));
			exit(EXIT_FAILURE);
		}else if(pid[i] == 0){
			sprintf(args[5], "%d", i);
			sprintf(args[6], "%d", shm_ids[i]); /* The shared-memory identifies */
			sprintf(args[0], "elevator");
			dup2(pm[i][0], STDIN_FILENO);/* Redirect stdin to pipe read */
			close(pm[i][1]); /* Close write*/
			if(execlp("./elevator",argv[0],args[1],args[2],args[3],args[4],args[5],args[6],NULL) < 0){
				fprintf(stderr, "execve error %s\n",
						strerror(errno));
			}
			exit(EXIT_FAILURE); 
		}else /* controller process */
			close(pm[i][0]); /* close read */
	
	}
	/* Create a thread for interaction */
	if(pthread_create(&thread, NULL, thread_handler, NULL) < 0){
		fprintf(stderr, "pthread_create error %s\n",strerror(errno));
	}                                                               

	pause();
	printf("please waiting.......\n");
	sleep(1); /* This is very important */	
	printf("controller startup\n");
	
	/* startup all elevator child process */
	for(i = 0; i < ELEVATOR_NUMBER ; i++){
		kill(pid[i], SIGUSR1);
	}

	current_time = 0;
	printf("dataset_count= %d\n", dataset_count);
	/* FSM start running */
	while(1){
		/* check data-set */
		if(dataset_index >= 0){
			request_t* req_ptr;
			while(dataset_index < dataset_count){
				req_ptr = &(dataset[dataset_index]);
				if(req_ptr->start_time != current_time)
					break;
				else{
					/* Scheduling or insert the wait-queue */
					human_count ++ ;
					dataset_index ++ ;
				    insert_queue(req_ptr, current_time);
				}
			}
		}
		/* Check the UP wait-queue and the down wait-queue */
		care_multi_request_queue(current_time);

		alarm(1);
		pause();
		sem_wait(&local_mutex);
		current_time ++;
		sem_post(&local_mutex);
	}

}
                                                       
void* 
thread_handler(void* args){
	pthread_detach(pthread_self());
	pid_t pid = getpid();
	char buf[BUFSIZE];
	printf("**** Welcome to Rock's elevators-system! ****\n");
	printf("**** You can type some commands (help = ?) ****\n");
	while(1){
		printf(">> ");
		if((fgets(buf, BUFSIZE, stdin)) == NULL){         	
        	if(errno == EINTR) /* Interrupted by signal */
        		continue;
        	else{
        		fprintf(stderr,"read error\n");
        		break;
        	}
        }
        buf[strlen(buf)-1] = '\0';                       
		if(strlen(buf) == 0)
			continue;
		if(strcmp(buf, "startup") == 0){
			kill(pid, SIGUSR2);
		}
		else if(strcmp(buf, "quit") == 0){
			printf("quit done.\n");
			kill(pid*(-1), SIGINT);
			break;
		}else if(strcmp(buf, "?") == 0){
			printf("   <startup> Startup controller and elevators\n");
			printf("   <quit> Quit this interaction thread\n");
			printf("   <ele> Print the state of all elevator \n");
			printf("   <rq> Print the request queue \n");
			printf("   <d>  Dynamic show all state information\n");
			printf("   <?> Help\n");
		}
		else if(strcmp(buf, "ele") == 0)
			for(int i = 0; i < ELEVATOR_NUMBER; i++){
				print_elevator_state(i);
				printf("\n");
			}
		else if(strcmp(buf, "rq") == 0){
			print_request_queue();
		}
		else if(strcmp(buf, "d") == 0){
			print_dynamic_state_table();
		}
		else
			printf("%s invaild command\n", buf);
		
	}
	return NULL;
}

void 
print_request_queue(){
	/* first level queue */
	priority_queue_t* level_rq;
	for(int i = 0; i < 3 ; i++){
		level_rq = multi_request_queue_get_pq(&my_multi_rq,i);
		if(level_rq == NULL) {
				printf("multi_request_queue_get_pq return null \n");
				continue;
		}
		priority_queue_move_tail(level_rq);
		request_t* tmp_value = (request_t*)priority_queue_next(level_rq);
		while(tmp_value != NULL){
			tmp_value = (request_t*)tmp_value;
			printf("{%d %d %d}", tmp_value->start_time,
									tmp_value->source_floor,
									tmp_value->target_floor);
			tmp_value = (request_t*)priority_queue_next(level_rq);
		}
		printf("\n");
	}
	priority_queue_move_tail(level_rq);
}
void 
print_elevator_state(int ele_index){
	if(ele_index >= ELEVATOR_NUMBER)
		return;
	elevator_shared_t tmp;
	int front, end, offset;
	char buf[BUFSIZE];
	char human_buf[MAX_HUMAN_EACH_ELE+1];
	
	buf[0] = '\0';
	human_buf[0] = '\0';
	//printf("brfore shared\n");
	multi_shared_get_elevator_shared(&my_multi_shared,
										&tmp,
										ele_index);
	printf(" Elevator %d: ", ele_index);
	int n = (tmp.target_floor_queue)[0];
    for(int k = 1; k < n + 1; k++)
    	sprintf(buf,"%s %d", buf, (tmp.target_floor_queue)[k]);
			
	offset = tmp.current_floor_number + tmp.directive;
	front = MIN(tmp.current_floor_number, offset);
	end = MAX(tmp.current_floor_number, offset);
	if(tmp.current_state == SLEEPING){
		sprintf(human_buf, "-");
	}else{
		for(int j = 0; j < tmp.container_human_number ; j++)
			sprintf(human_buf, "%s|", human_buf);
	}
	if(tmp.directive == STOP){
		printf("%d {%s} %d", front, human_buf, end);
		printf(" *** %s", buf);
	}else if(tmp.directive == UP){
		printf("%d {%s}> %d", front, human_buf, end);
		printf(" *** %s ", buf);
	}else{
		printf("%s *** ", buf);
		printf("%d <{%s} %d",front, human_buf, end);
	}
	//printf("\n");
	
}
void 
print_dynamic_state_table(){
	char buf[BUFSIZE]	;
	//fd_set read_set, write_set;
	//FD_ZERO(&read_set); /* Clear read set*/
	/*FD_ZERO(&write_set);
	FD_SET(STDIN_FILENO, &read_set);
	FD_SET(STDOUT_FILENO, &write_set);
	struct timespec timeout;
	timeout.tv_nsec  = 1000;
	timeout.tv_sec = 0;*/
	
	elevator_shared_t es_array[ELEVATOR_NUMBER];
	int human_count_each_elelvator[MAX_FLOOR_NUMBER+1];
	priority_queue_t * level_rq;
	request_t* req_ptr;
	int max_show_human = 25;
	while(1){
		memset(human_count_each_elelvator, (int)0, sizeof(int)*
						(MAX_FLOOR_NUMBER+1));
		for(int i = 0 ; i < 3 ; i++){
			sem_wait(&local_mutex);
			if((level_rq = multi_request_queue_get_pq(&my_multi_rq, i)) != NULL){
        		priority_queue_move_tail(level_rq);
       		 	req_ptr = (request_t*)priority_queue_getvalue(level_rq);
                while(req_ptr != NULL){
					human_count_each_elelvator[req_ptr->source_floor] ++;
					priority_queue_next(level_rq);
					req_ptr = (request_t*)priority_queue_getvalue(level_rq);
				}
        		priority_queue_move_tail(level_rq);
            }
			sem_post(&local_mutex);
		}
		for(int i = 0; i < ELEVATOR_NUMBER; i++){
			multi_shared_get_elevator_shared(&my_multi_shared, &es_array[i],i);
			//printf("%d ", es_array[i].current_floor_number);
		}
		printf("Floor : ");
		
		sem_wait(&local_mutex);
		int tmp_current_time = current_time;
		sem_post(&local_mutex);
		sprintf(buf,"Current Time: %d", tmp_current_time);
		printf("%*s",20, buf);
		printf("%*s",9, " ");
		for(int i = 0 ; i< ELEVATOR_NUMBER; i++)
			printf(" Ele %d     ", i);
		printf("\n\n");
		for(int i = MAX_FLOOR_NUMBER; i >= 1 ;i-- )	{
			buf[0]='\0';
			printf("%.2d: ", i);
			int n = MIN(human_count_each_elelvator[i], max_show_human);
			//for(int j = 0; j < n; j++)
			//	sprintf(buf, "%s|",buf);
			memset(buf, (char)'|', sizeof(char)*n);
			buf[n] = '\0';
			if(n < human_count_each_elelvator[i]){
				sprintf(buf, "%s...",  buf);
				//buf[sizeof()]
			}
			printf("%-*s", 28,buf);
			printf("%*s",5," ");
			
			for(int k = 0 ; k < ELEVATOR_NUMBER; k++){
				printf("|");
				if(es_array[k].current_floor_number == i){
					buf[0]='\0';
					int chn = es_array[k].container_human_number;
					if(chn > 0){
						memset(buf, (char)'*', 
							sizeof(char)*chn);
						buf[chn]  = '\0';
					}else{
						memset(buf, (char)'-', 
							sizeof(char)*MAX_HUMAN_EACH_ELE);
						buf[MAX_HUMAN_EACH_ELE] = '\0';
					}
					printf("%*s", MAX_HUMAN_EACH_ELE, buf);
						
				}else{
					printf("%*s", MAX_HUMAN_EACH_ELE, " ");
				}
				printf ("|");
				printf("   ");
			}
			printf("\n");
		}
		/*int fd = pselect(STDERR_FILENO, &read_set, &write_set,NULL
						NULL, NULL);
		if(fd == STDOUT_FILENO){
			printf("HELLO");	
		
		}else if(fd == STDIN_FILENO){
			if(fgets(buf,  BUFSIZE, stdin) != NULL){
				buf[strlen(buf)-1] = '\0';
				if(strcmp(buf, "q") == 0)
					break;
			}
		}*/
		sleep(3);
	}
}
void                                                                                                                                                   
insert_queue(request_t* req_ptr, int current_time){	
 	int dir = 0;
 	if(req_ptr->source_floor < req_ptr->target_floor)
 		dir = 1;
 	else if(req_ptr->source_floor > req_ptr->target_floor)
 		dir = -1;
 	else{
 		fprintf(stderr, "invaild data in dataset time: %d\n", req_ptr->start_time);
 		return; /* error */
 	}
 	//printf("dir = %d\n", dir);
 	if(my_sche_strategy != NORMAL && (req_ptr->target_floor == 1
 							    || req_ptr->source_floor == 1))
 		multi_request_queue_insert(&my_multi_rq, 0, req_ptr);
 	else if(dir == 1)
 		multi_request_queue_insert(&my_multi_rq, 1, req_ptr);
 	else
 		multi_request_queue_insert(&my_multi_rq, 2, req_ptr);
 	/*printf("%d s insert one request {%d %d %d}\n", current_time,
 											req_ptr->start_time,
 											req_ptr->source_floor,
 											req_ptr->target_floor);*/
 }                                                                                     

void
care_multi_request_queue(int current_time){
	priority_queue_t* level_rq;
	int res;
	request_t* tmp_request;
	int care_requests_each_elevator[ELEVATOR_NUMBER];
//	memset(care_requests_each_elevator, (int)0, sizeof(int)*ELEVATOR_NUMBER);
	for(int k = 0 ; k < ELEVATOR_NUMBER ; k++)
		care_requests_each_elevator[k] = 0;
	request_ptr_t care_requests[ELEVATOR_NUMBER][MAX_HUMAN_EACH_ELE];
	

	//printf("Care :%d s\n", current_time);
	for(int i = 0; i < 3; i++){
		sem_wait(&local_mutex);
		level_rq = multi_request_queue_get_pq(&my_multi_rq, i);
		if(level_rq == NULL) {
			sem_post(&local_mutex);
			continue;
		};
		priority_queue_move_tail(level_rq);
		tmp_request = (request_t*)priority_queue_getvalue(level_rq);
		while(tmp_request != NULL){
			/*printf(" {, %d %d}\n ", tmp_request->source_floor
							, tmp_request->target_floor);*/

			res = schedule(&my_schedule, tmp_request, 
							care_requests_each_elevator);

			if(res < 0){ /*refuse schedule */
				//if(res == -1)
				//	break;
				//else
				priority_queue_next(level_rq);
			}
			else{
				int n = tmp_request->is_preprocessing;
            	(care_requests[n][care_requests_each_elevator[n]]).req_ptr = tmp_request;
            	care_requests_each_elevator[n] ++;
				if(res == 0){  /* preprocessing */
					//send_human_to_elevator(tmp_request,1);
            		tmp_request->is_preprocessing = 1;
					priority_queue_next(level_rq);
				}else{ /* Finished */
					//send_human_to_elevator(tmp_request, 0);
            		tmp_request->is_preprocessing = 0;
					priority_queue_remove(level_rq, 0);
				}
			}
			tmp_request = (request_t*)priority_queue_getvalue(level_rq);

		}
		priority_queue_move_tail(level_rq);
		sem_post(&local_mutex);
	}

	send_human_to_elevator(care_requests_each_elevator, care_requests);
}
void
send_human_to_elevator(int* care_requests_each_elevator,
						request_ptr_t care_requests[][MAX_HUMAN_EACH_ELE]){
	char buf[50];
	int start_time, source_floor, target_floor, n;
	request_t* req_ptr = NULL;
	for(int i = 0 ; i < ELEVATOR_NUMBER; i++){
		if((n = care_requests_each_elevator[i]) <= 0){
			continue;
		}else if(n > MAX_HUMAN_EACH_ELE){
			printf("OUT OF BOUND i = %d  n = %d\n", i, n);
		}
		buf[0] = '\0';
		for(int j = 0; j < n ; j++){
			if((req_ptr = ((care_requests[i][j]).req_ptr)) != NULL){
				if(req_ptr->is_preprocessing){
            		start_time = target_floor = -1;
            		source_floor = req_ptr->source_floor;
            	}else{
            		start_time = req_ptr->start_time;
            		source_floor = req_ptr->source_floor;
            		target_floor = req_ptr->target_floor;
            	}
			 	sprintf(buf,"%s%d %d %d:", buf,start_time,     
             						  	source_floor,
             						 	target_floor);
			}
		}
		int size = strlen(buf);
		if(size > 0)
			buf[size-1] = '\n';
		write(pm[i][1], buf, size);
		//print_elevator_state(i);
		//printf("  index : %d ==>  %s ",i , buf);
	}
}
void 
sigint_handler(int sig){
	for(int i = 0; i < ELEVATOR_NUMBER; i++)
		close(pm[i][1]);
	/* Wait child */
		
	pid_t tmp_pid;
	while((tmp_pid = waitpid(-1, NULL,0)) > 0){
		printf("child %ld terminated\n", (long)tmp_pid);
	}
	/*Free shared momory */
	multi_shared_free(&my_multi_shared);
	multi_request_queue_free(&my_multi_rq);
	exit(EXIT_SUCCESS);
}
                                                       
