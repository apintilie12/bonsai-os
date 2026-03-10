#include "kernel/sched.h"
#include "kernel/irq.h"
#include "lib/printf.h"
#include "arch/utils.h"
#include "mm/mm.h"
#include "lib/spinlock.h"

static TASK_STRUCT init_task = INIT_TASK;
static TASK_STRUCT idle_tasks[4]; // [0] unused; cores 1-3 use [1..3]
CPU_INFO cpu_data[4];
LIST_ENTRY global_all_threads_list;
int nr_tasks = 1;
static spinlock_t sched_lock = SPINLOCK_INIT;


void sched_init(void) 
{
	// Setup Core 0
	cpu_data[0].current_task = &init_task;
	cpu_data[0].cpu_id = 0;
	asm volatile("msr tpidr_el1, %0" :: "r"(&cpu_data[0]));

	list_head_init(&global_all_threads_list);
	list_add_tail(&global_all_threads_list, &(init_task.all_threads_list));
}

void sched_init_secondary(int cpu_id)
{
	TASK_STRUCT *idle = &idle_tasks[cpu_id];
	idle->state = TASK_RUNNING;
	idle->counter = 1;
	idle->priority = 1;
	idle->preempt_count = 0;
	idle->flags = PF_KTHREAD;
	idle->on_cpu = cpu_id;
	idle->mm.pgd = 0;

	const char *prefix = "idle-";
	int i;
	for (i = 0; prefix[i]; i++)
		idle->name[i] = prefix[i];
	idle->name[i] = '0' + cpu_id;
	idle->name[i+1] = '\0';

	cpu_data[cpu_id].current_task = idle;
	// Not added to global_all_threads_list — idle tasks are core-local only
}

void add_task(TASK_STRUCT *task) {
	spin_lock(&sched_lock);
	list_add_tail(&global_all_threads_list, &task->all_threads_list);
	spin_unlock(&sched_lock);
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
	TASK_STRUCT *current = get_current_task();
	int cpu_id = get_cpu_info()->cpu_id;
	spin_lock(&sched_lock);
	while (1) {
		c = -1;
		next = 0;
		LIST_FOR_EACH_ENTRY(p, &global_all_threads_list, all_threads_list) {
			if (p && p->state == TASK_RUNNING && p->on_cpu == -1 && p->counter > c) {
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
	if (next && next != current) {
		// current->on_cpu = -1;
		next->on_cpu = cpu_id;
	} else {
		next = 0;
	}
	spin_unlock(&sched_lock);
	if (next) {
		switch_to(next);
	}
	preempt_enable();
}

void schedule(void)
{
	get_current_task()->counter = 0;
	_schedule();
}


void switch_to(TASK_STRUCT *next)
{
	TASK_STRUCT *prev = get_current_task();
	get_cpu_info()->current_task = next;
	set_pgd(next->mm.pgd);
	prev->on_cpu = -1;
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
	// enable_irq();
	_schedule();
	// disable_irq();
}

void exit_process(){
	preempt_disable();
	TASK_STRUCT *p;
	TASK_STRUCT *current = get_current_task();
	spin_lock(&sched_lock);
	LIST_FOR_EACH_ENTRY(p, &global_all_threads_list, all_threads_list) {
		if (p == current) {
			p->state = TASK_ZOMBIE;
			break;
		}
	}
	spin_unlock(&sched_lock);
	preempt_enable();
	schedule();
}