#include "sched.h"
#include "irq.h"
#include "printf.h"
#include "utils.h"
#include "mm.h"

static TASK_STRUCT init_task = INIT_TASK;
CPU_INFO cpu_data[4];
LIST_ENTRY global_all_threads_list;
int nr_tasks = 1;


void sched_init(void) 
{
	// Setup Core 0
	cpu_data[0].current_task = &init_task;
	cpu_data[0].cpu_id = 0;
	asm volatile("msr tpidr_el1, %0" :: "r"(&cpu_data[0]));

	list_head_init(&global_all_threads_list);
	list_add_tail(&global_all_threads_list, &(init_task.all_threads_list));
}

void preempt_disable(void)
{

	get_current_task()->preempt_count++;
}

void preempt_enable(void)
{
	get_current_task()->preempt_count--;
}


void _schedule(void)
{
	preempt_disable();
	int c;
	TASK_STRUCT *next;
	TASK_STRUCT *p;
	while (1) {
		c = -1;
		next = 0;
		LIST_FOR_EACH_ENTRY(p, &global_all_threads_list, all_threads_list) {
			if (p && p->state == TASK_RUNNING && p->counter > c) {
				c = p->counter;
				next = p;
			}
		}
		if (c) {
			break;
		}
		LIST_FOR_EACH_ENTRY(p, &global_all_threads_list, all_threads_list) {
			if (p) {
				p->counter = (p->counter >> 1) + p->priority;
			}
		}
	}
	switch_to(next);
	preempt_enable();
}

void schedule(void)
{
	get_current_task()->counter = 0;
	_schedule();
}


void switch_to(TASK_STRUCT *next) 
{
	TASK_STRUCT* current = get_current_task();
	if (current == next)
		return;
	TASK_STRUCT *prev = current;

	get_cpu_info()->current_task = next;

	set_pgd(next->mm.pgd);
	cpu_switch_to(prev, next);
}

void schedule_tail(void) {
	preempt_enable();
}


void timer_tick()
{
	TASK_STRUCT* current = get_current_task();
	--current->counter;
	if (current->counter>0 || current->preempt_count >0) {
		return;
	}
	current->counter=0;
	enable_irq();
	_schedule();
	disable_irq();
}

void exit_process(){
	preempt_disable();
	TASK_STRUCT *p;
	TASK_STRUCT *current = get_current_task();
	LIST_FOR_EACH_ENTRY(p, &global_all_threads_list, all_threads_list) {
		if (p == current) {
			p->state = TASK_ZOMBIE;
			break;
		}
	}
	preempt_enable();
	schedule();
}