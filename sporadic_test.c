#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <sys/mman.h>

#include <native/task.h>
#include <native/timer.h>
#include <rtdk.h>

#include "edfomai-app.h"

#define NWAIT_SESSION 5
#define WASTE_TIME 360000 // 360 000 ns
#define WAIT_SESSION  (int) WASTE_TIME/NWAIT_SESSION


RT_TASK spawner;
RT_TASK t1;
RT_TASK t2;
RT_TASK t3;
void demo (void *arg){
	int i;
	RT_TASK *curtask;
	RT_TASK_INFO curtaskinfo;
	
	curtask = rt_task_self();
	rt_task_inquire(curtask,&curtaskinfo);
	
	rt_printf("!%s started\n", curtaskinfo.name);
	
	for ( i=0 ; i < WASTE_TIME; i+=WAIT_SESSION ){
		rt_timer_spin(WAIT_SESSION);
		rt_task_inquire(curtask,&curtaskinfo);
		rt_printf("->%s @ prio %d\n", curtaskinfo.name, curtaskinfo.cprio);
	}

		rt_printf("]%s terminating\n", curtaskinfo.name);
}
int main (int argc, char * argv[]){
	int ret=0;
	char str0[10];
	char str1[10];
	char str2[10];
	char str3[10];
	unsigned long deadline1;
	unsigned long deadline2;
	unsigned long deadline3;

	deadline1=200000;	//nsec
	deadline2=350000;	//nsec
	deadline3=250000;
	sprintf(str0,"SPAWNER");
	sprintf(str1,"T1");
	sprintf(str2,"T2");
	sprintf(str3,"T3");
	
	rt_print_auto_init(1);
	mlockall(MCL_CURRENT|MCL_FUTURE);
	
	rt_task_shadow(&spawner, str0,50,T_CPU(0));
	
	rt_printf(">Starting edfomai..\n");
	ret=edf_init();
	if (ret)
			return -1;
	rt_printf(" Done!\n");
	rt_printf(">Creating tasks\n");
	ret=rt_task_create(&t1,str1,0,25,0);
	ret=rt_task_create(&t2,str2,0,25,0);
	ret=rt_task_create(&t3,str3,0,25,0);
	rt_printf(">Starting tasks\n");
	ret=edf_task_start(&t1,deadline1,&demo,0);
	ret=edf_task_start(&t2,deadline2,&demo,0);
	ret=edf_task_start(&t3,deadline3,&demo,0);
	
	rt_task_join(&t2);
	rt_task_join(&t1);
	rt_task_join(&t3);
	rt_printf(">Disposing edfomai..\n");
	ret=edf_dispose();
	rt_printf(" Done!\n");
	return 0;
}


