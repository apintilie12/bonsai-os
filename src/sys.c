#include "fork.h"
#include "log.h"
#include "utils.h"
#include "sched.h"
#include "mm.h"


void sys_write(char * buf){
	LOG_CORE("%s", buf);
}

int sys_fork(){
	return copy_process(0, 0, 0, "some-name");
}

void sys_exit(){
	exit_process();
}

void * const sys_call_table[] = {sys_write, sys_fork, sys_exit};
