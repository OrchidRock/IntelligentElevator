/*	elevator.c
 *	Describe:
 *		This is the elevetor module. It shall be 
 *		implemented by a FSM(finite-state machine)
 *
 */
#include "common.h"
#include "elevator_floor_queue.h"
#include "elevator_container.h"
#include "elevator_shared.h"
/* Three states of FSM */
/* See common.h SLEEPING, RUNNING, ARRIVED */

/* Local functions' prototype */
void* thread_handler(void*); 
void siguser1_handler(int sig){} /* Do nothing */
void sigalrm_handler(int sig); /* write log */
void sigint_handler(int sig);
void logging(char*);
void print_floor_queue();
void increment_cfn();
void decrement_cfn();
int get_cfn();
int get_dir_by_current_fn(int*);

/* Global varible */
int max_floor_number; /* The max number of floor 
					   come from controller */

floor_queue_t floors_queue;
container_t container;
shared_t my_shared;

FILE* log_fd;
//volatile stop_sleeping = 0;
volatile elevator_state_enum current_state;

volatile int current_floor_number;
sem_t local_mutex ;

time_t startup_time; /* synchronous with controller */
//time_t current_time;
long total_time;
int total_human;
int current_time;

/* argv[1] the number of current elevator
 * argv[2] running one floor time
 * argv[3] stop on one floor time
 * argv[4] 
 */
int 
main(int argc, char* argv[]){

	if(argc < 6){
		fprintf(stderr, "Error: elevator process startup failed because of invaild arguments() \n");
		exit(EXIT_FAILURE);
	}
	/* Init the important variables */	
	int ele_number = 0; /* The default number of current elevator */
	int run_one_floor_time = 3; /* The unit of time is sec */
	int stop_one_floor_time = 10;
	int is_moudle_test = 1;
	pthread_t thread;
	char buf[BUFSIZE];


	run_one_floor_time = atoi(argv[1]);
	stop_one_floor_time = atoi(argv[2]);
	max_floor_number = atoi(argv[3]);
	int container_max_human_number = atoi(argv[4]);
	ele_number = atoi(argv[5]);
	/* Shared memory */
	if(argv[6] != NULL){
		shared_init(&my_shared, atoi(argv[6]));
		is_moudle_test = 0;
	}else{
		shared_init(&my_shared, -1);
	}
	

	current_state = SLEEPING ; /* Init for SLEEPING state */
	current_floor_number = 1 ; /* Default sleeping on floor 1 */

	/* Init queue and log_fd and container */
	if(floor_queue_init(&floors_queue) < 0){
		fprintf(stderr, "floor_queue_init error\n");
	}

	if(container_init(&container, container_max_human_number) < 0){
		fprintf(stderr, "container_init error\n");
	}
	shared_all_init(&my_shared, current_floor_number);
	sem_init(&local_mutex, 0, 1);
	sprintf(buf, "../log/elevator_%d.log", ele_number);
	if(is_moudle_test)
		log_fd = stderr;
	else{
		log_fd = fopen(buf, "w");
		if(log_fd == NULL){
			fprintf(stderr, "elevator %d : open log file error\n",
							ele_number);
			exit(EXIT_FAILURE);
		}
	}
	/* Install signal handler */
	if(signal(SIGUSR1, siguser1_handler) == SIG_ERR){
		fprintf(log_fd, "signal error\n");
	}
	if(signal(SIGALRM, sigalrm_handler) == SIG_ERR){
		fprintf(log_fd, "signal error\n");
	}
	if(signal(SIGINT, sigint_handler) == SIG_ERR)
		fprintf(log_fd, "signal error\n");

	 /*Create a thread for communicated with controller */
	int res = pthread_create(&thread, NULL, thread_handler, NULL);
	if(res != 0){
		fprintf(log_fd, "elevetor %d : pthread_create error\n", ele_number);
		exit(EXIT_FAILURE);
	}

	/* Waiting for controller signal */
	if(!is_moudle_test) pause();
	printf("elevator %d startup pid: %d\n", ele_number, (int)getpid());	
	startup_time = time((time_t *)0);
	int start_time_array[container_max_human_number];
	total_time = 0;
	current_time = 0;
	/* Loop running */
	while(1){					
		if(current_state == SLEEPING){
			shared_update_directive(&my_shared, STOP);
			logging(NULL);
			pause();     /* Waiting thread weak up */
			current_state = RUNNING;

			current_time = (int)difftime(time((time_t*)0), startup_time);
			//logging(NULL);
			//alarm(run_one_floor_time); /* Start alarm */
		}else if(current_state == RUNNING){
			logging(NULL);
			int floor_value = floor_queue_getvalue(&floors_queue);
			if(floor_value < 0) {
				current_state = SLEEPING;
				shared_update_curr_state(&my_shared, SLEEPING);
				continue;
			}
			
			if(floor_value == current_floor_number){
				current_state = ARRIVED;
				shared_update_directive(&my_shared,
								floor_queue_get_directive(&floors_queue));
				shared_update_curr_floor(&my_shared, current_floor_number);
				shared_update_curr_state(&my_shared, ARRIVED);

				//alarm(stop_one_floor_time);
				continue;
			}else if(floor_value < current_floor_number){ /* DOWN */
				shared_update_directive(&my_shared, DOWN);
				shared_update_curr_floor(&my_shared, current_floor_number);
				decrement_cfn();
			}
			else{  /* UP */
				shared_update_directive(&my_shared, UP);
				shared_update_curr_floor(&my_shared, current_floor_number);
				increment_cfn();
				//current_floor_number ++;
			}
			shared_update_curr_state(&my_shared, RUNNING);
			alarm(run_one_floor_time);
			pause();
			current_time += run_one_floor_time;	
		}else{ /* ARRIVED state */
			logging(NULL);
			alarm(stop_one_floor_time);
			floor_queue_remove(&floors_queue);
			/* Remove human */
			int count = container_remove(&container, start_time_array, current_floor_number);
			fprintf(log_fd, "Have %d humans down\n",count);
			if(count > 0){
				for(int i = 0; i < count ; i++){
					total_time += current_time - start_time_array[i];
				}
				shared_update_contain_hn(&my_shared, 
								container_current_human_number(&container));
			}
			pause();
			current_time += stop_one_floor_time;

			if(floor_queue_getvalue(&floors_queue) < 0){ /* floors_link's next is null*/
				current_state = SLEEPING;
				shared_update_curr_state(&my_shared, SLEEPING);
			}
			else{
				current_state = RUNNING;
				shared_update_curr_state(&my_shared, RUNNING);
			}
		}
				
	}
	//pthread_join(thread, NULL);
	/*fflush(log_fd);
	fclose(log_fd);
	link_free(&floors_link);
	*/
	exit(EXIT_SUCCESS);
}
void* 
thread_handler(void * args){
	pthread_detach(pthread_self());
	//int nread;
	char buf[BUFSIZE];
	int command_floors[max_floor_number+1];
	pid_t pid = getpid();
	setvbuf(stdin, NULL, _IONBF, 0);
	//printf("Thread startuped\n");
	while(1){

		if((fgets(buf, BUFSIZE, stdin)) == NULL){
			if(errno == EINTR) /* Interrupted by signal */
				continue;
			else{
				fprintf(log_fd,"read error\n");
				break;
			}
		}
		buf[strlen(buf)-1] = '\0'; 
		fprintf(log_fd,"elevator buf = %s\n", buf);
		if(strlen(buf) == 0)
			continue;
		if(strcmp(buf, "quit") == 0)
			break;
		//write
		char* delimt;
		char* delimt_out = NULL;
		//char* tmp_ptr;	
		char* tmp_ptr2 = buf;
		human_t local_human;
		int index = 1;
		int human_start_time = -1;
		int human_target_floor = -1;
		int hum_source_floor = -1;
		//printf("Test:\n");
	
		do{
			index = 1;
			if(delimt_out != NULL){
				*delimt_out = '\0';
				tmp_ptr2 = delimt_out + 1;
			}
			/* Get start_time */
			if((delimt = strstr(tmp_ptr2, " ")) != NULL){
				*delimt = '\0';
				human_start_time = atoi(tmp_ptr2);
				tmp_ptr2 = delimt + 1;
			}
			/* Get source floor */
			if((delimt = strstr(tmp_ptr2, " ")) != NULL){
				*delimt = '\0';
				hum_source_floor= atoi(tmp_ptr2) % (max_floor_number+1);
				if(hum_source_floor > 0){
					if(current_state != ARRIVED || hum_source_floor != get_cfn())
						command_floors[index++] = hum_source_floor;
				}
				tmp_ptr2 = delimt + 1;
			}
            /* Get target floor */                      
			human_target_floor = atoi(tmp_ptr2) % (max_floor_number+1);
			if(human_target_floor > 0)
				command_floors[index++] = human_target_floor;

			command_floors[0] = index-1;
			if(human_start_time >= 0){	
				local_human.target_floor = human_target_floor;
				local_human.start_time = human_start_time;
				int cn;
				if((cn = container_add(&container, &local_human)) < 0){
					logging("container_add failed!\n");
					continue;
				}
				shared_update_contain_hn(&my_shared, cn);
				fprintf(log_fd, "%d s has a human that go to floor %d up\n", local_human.start_time,
				local_human.target_floor);
				total_human ++;
			}
			int dir = get_dir_by_current_fn(command_floors);
			floor_queue_insert(&floors_queue, command_floors, dir);

		}while((delimt_out = strstr(tmp_ptr2, ":")) != NULL);

		print_floor_queue(); /* shall invoke shared_update_floor_link */
		if(current_state == SLEEPING) kill(pid, SIGUSR1); /* Wake up elevetor */
		

		/*UNLOCK */

		//printf("CC::w ");
		//for(int i = 1; i < command_floors[0] + 1 ; i++)
		//	printf("%d ", command_floors[i]);
		//printf("\n");
	}
	fflush(log_fd);
	//pthread_exit(0);
	return NULL;
}
void sigalrm_handler(int sig){
	//logging(NULL);
}

void sigint_handler(int sig){
	//printf("Good Bay!\n");
	fprintf(log_fd,"total time : %ld\n", total_time);
	fprintf(log_fd,"total human : %d\n", total_human);

	fflush(log_fd);
	shared_update_total_time(&my_shared, total_time);
	//printf("Here1\n");
	floor_queue_free(&floors_queue);
	container_free(&container);
	shared_detach(&my_shared);
	fclose(log_fd);
	close(STDIN_FILENO);
	exit(EXIT_SUCCESS);
}

void logging(char* msg){
	char buf[BUFSIZE];
	if(msg != NULL){
		sprintf(buf, "%s", msg);
	}
	else{
		//printf("Alarm!\n");
		sprintf(buf, "State ===> Time: %d s, Floor: %d, State: %d\n",
				current_time, 
				current_floor_number,
				current_state);
	}
	fprintf(log_fd, buf);
	fflush(log_fd);                                       
}
void print_floor_queue(){
	char buf[BUFSIZE];
	int tmp_floor_queue[MFN+1];

	floor_queue_get_floor_queue(&floors_queue, tmp_floor_queue);
	shared_update_floor_link(&my_shared,tmp_floor_queue);

	logging("Link data: ");
	for(int i = 0; i < tmp_floor_queue[0]; i++){
		sprintf(buf, "%d ", tmp_floor_queue[i+1]);
		logging(buf);
	}                                         
	logging("\n");    
}
inline void increment_cfn(){
	sem_wait(&local_mutex);
	current_floor_number ++;
	sem_post(&local_mutex);
}
inline void decrement_cfn(){
	sem_wait(&local_mutex);
	current_floor_number --;
	sem_post(&local_mutex);
}
inline int get_cfn(){
	int res;
	sem_wait(&local_mutex);
	res = current_floor_number;
	sem_post(&local_mutex);
	return res;
}   

inline int get_dir_by_current_fn(int * ca){
	int dir = 0;
	int cfn = get_cfn();
	if(ca[0] > 1){
		if(ca[1] < ca[2])
			dir = 1; /* UP */
		else if (ca[1] == ca[2])
			dir = 0;
		else
			dir = -1; /* DOWN */
	}else if (ca[0] == 1){
		if(cfn < ca[1]) /* UP*/
			dir = 1;
		else if(cfn == ca[1])
			dir = 0;
		else
			dir = -1;
	}
	return dir;

}
