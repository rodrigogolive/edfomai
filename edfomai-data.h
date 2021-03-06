#ifndef _EDFOMAI_DATA_
#define _EDFOMAI_DATA_

#ifdef MODULE
#include <native/task.h>
#else
#include <native/task.h>
#endif

#define TNAME_LEN 24
#define DEADLINENOTSET -1
#define DMISSEDQUEUE "EDF-DMISSED-Q"
#define DMISSEDQUEUE_SIZE 1024
/*
* Command Type
*/
typedef enum EDFCommand {
	CREATE_TASK=0,  	
	START_TASK=1,
	RESET_DEADLINE=2,
	SET_DEADLINE=3,
	STOP_WATCHD=4,
	GOING_WAITP=5,
} EDFCommand;
/*
* Message interpretable by the edfomai driver module
*/
typedef struct EDFMessage {
	EDFCommand command;
	char task [TNAME_LEN+1];
	unsigned long long deadline;
} EDFMessage;

#endif
