.PHONY: clean log

CC = gcc
LIBS = -lpthread
CFLAGS =  -g -Wall
SRC_DIR = ./src
BIN_DIR = ./bin
LOG_DIR = ./log

ELEVATOR_SRC = $(wildcard $(SRC_DIR)/elevator*.c)
ELEVATOR_SRC += $(SRC_DIR)/priority_queue.c 
#ELEVATOR_OBJS = elevator.o elevator_floor_queue.o elevator_container.o elevator_shared.o
#ELEVATOR_OBJS = $(patsubst %.c, %.o, $(ELEVATOR_SRC))
ELEVATOR_OBJS = $(ELEVATOR_SRC:.c=.o)
#ELEVATOR_OBJS += $(SRC_DIR)/priority_queue.o

#CTL_OBJS = controller.o controller_shared.o controller_schedule.o controller_request_queue.o
CTL_SRC = $(wildcard $(SRC_DIR)/controller*.c)
CTL_SRC += $(SRC_DIR)/priority_queue.c
CTL_OBJS = $(patsubst %.c, %.o, $(CTL_SRC))
#CTL_OBJS += priority_queue.o

HEADER_FILES = $(wildcard $(SRC_DIR)/*.h)

all: elevator controller

elevator: $(ELEVATOR_OBJS) $(LIBS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)


%.o:%.c $(SRC_DIR)/common.h
	$(CC) $(CFLAGS) -o $@ -c $<


controller: $(CTL_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

log:
	@test -d $(LOG_DIR) || \
	 { mkdir $(LOG_DIR);}
	
	@for x in 0 1 2 3; do \
	  touch $(LOG_DIR)/elevator_$$x.log; \
	done

clean:
	rm -f $(ELEVATOR_OBJS) $(CTL_OBJS) elevator controller
