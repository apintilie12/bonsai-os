#ifndef _SCHED_H
#define _SCHED_H

#include "list.h"

#define THREAD_CPU_CONTEXT			0 		// offset of cpu_context in task_struct 

#ifndef __ASSEMBLER__

#define THREAD_SIZE				4096

#define TASK_RUNNING				0
#define TASK_ZOMBIE					1

#define TASK_NAME_MAX_LEN 			64

#define PF_KTHREAD		            	0x00000002	

typedef struct _CPU_CONTEXT {
	unsigned long x19;
	unsigned long x20;
	unsigned long x21;
	unsigned long x22;
	unsigned long x23;
	unsigned long x24;
	unsigned long x25;
	unsigned long x26;
	unsigned long x27;
	unsigned long x28;
	unsigned long fp;
	unsigned long sp;
	unsigned long pc;
} CPU_CONTEXT;

#define MAX_PROCESS_PAGES			16	

typedef struct _USER_PAGE {
	unsigned long phys_addr;
	unsigned long virt_addr;
} USER_PAGE;

typedef struct _MM_STRUCT {
	unsigned long pgd;
	int user_pages_count;
	USER_PAGE user_pages[MAX_PROCESS_PAGES];
	int kernel_pages_count;
	unsigned long kernel_pages[MAX_PROCESS_PAGES];
} MM_STRUCT;

typedef struct _TASK_STRUCT {
	CPU_CONTEXT cpu_context;
	long state;
	long counter;
	long priority;
	long preempt_count;
	unsigned long flags;
	int on_cpu;		// -1 = not currently running, 0-3 = running on that core
	MM_STRUCT mm;
	char name[TASK_NAME_MAX_LEN];
	LIST_ENTRY all_threads_list;
} TASK_STRUCT;

typedef struct _CPU_INFO {
	TASK_STRUCT *current_task;
	int cpu_id;
} CPU_INFO;

extern CPU_INFO cpu_data[4];


extern LIST_ENTRY global_all_threads_list;

static inline CPU_INFO* get_cpu_info(void) {
	CPU_INFO *ci;
	asm volatile("mrs %0, tpidr_el1" : "=r"(ci));
	return ci;
}

static inline TASK_STRUCT* get_current_task(void) {
	return get_cpu_info()->current_task;
}

extern int nr_tasks;


extern void sched_init(void);
extern void schedule(void);
extern void timer_tick(void);
extern void preempt_disable(void);
extern void preempt_enable(void);
extern void switch_to(TASK_STRUCT* next);
extern void cpu_switch_to(TASK_STRUCT* prev, TASK_STRUCT* next);
extern void exit_process(void);
extern void add_task(TASK_STRUCT* task);
extern void sched_init_secondary(int cpu_id);

#define INIT_TASK \
/* cpu_context */	{ {0,0,0,0,0,0,0,0,0,0,0,0,0}, \
/* state etc   */	0,0,15,0,PF_KTHREAD, \
/* on_cpu      */	0, \
/* mm */			{0, 0, {{0}}, 0, {0}}, \
/* name        */	"main-thread" \
}

#endif
#endif