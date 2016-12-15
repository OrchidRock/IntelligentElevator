#ifndef __COMMAND_H__
#define __COMMAND_H__
#define _GNU_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/select.h>
#include <semaphore.h>
#define BUFSIZE 1024 
#define MFN 20
#define MAX(a, b) (((a) < (b))?(b):(a))
#define MIN(a, b) (((a) > (b))?(b):(a))
/* human data struct */
typedef struct{
	int start_time ;/* The time when human come */
	int target_floor;/* The target floor human will go */
}human_t;                                                   

/* The request for human */
typedef struct{
	int start_time;
	int source_floor;
	int target_floor;
	int is_preprocessing ; /* Flag controller whether
							 preprocessing this request:w */
}request_t;

typedef enum{
	SLEEPING = 0,	/* Elevetor has nothing to do */
	RUNNING,	/* Elevetor is running between floor A and neigh-floor B */
	ARRIVED		/* Elevetor arrive one floor and open-close */
}elevator_state_enum;
typedef enum{
	DOWN = -1,
	STOP,
	UP
}directive_enum;
/* elevator state data */

typedef struct{
	int container_human_number; /* Current human number of elevator's container */
	elevator_state_enum current_state;	/* Current elevator state */
	directive_enum directive; /* 0 : UP  1 : DOWN -1:STOP */
	int current_floor_number;
	int target_floor_queue[MFN+1]; /* */
	long total_time; /* Send to controller for statistical analysis */
}elevator_shared_t;

#endif
