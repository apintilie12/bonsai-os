#include "mm.h"
#include "sched.h"
#include "entry.h"
#include "printf.h"
#include "fork.h"
#include "utils.h"
#include "list.h"

int copy_process(unsigned long clone_flags, unsigned long fn, unsigned long arg, const char *name)
{
	preempt_disable();
	TASK_STRUCT *p;

	unsigned long page = allocate_kernel_page();
	p = (TASK_STRUCT*) page;
	struct pt_regs *childregs = task_pt_regs(p);

	if (!p)
		return -1;

	if (clone_flags & PF_KTHREAD) {
		p->cpu_context.x19 = fn;
		p->cpu_context.x20 = arg;
	} else {
		struct pt_regs * cur_regs = task_pt_regs(current);
		*childregs = *cur_regs;
		childregs->regs[0] = 0;
		copy_virt_memory(p);
	}
	p->flags = clone_flags;
	p->priority = current->priority;
	p->state = TASK_RUNNING;
	p->counter = p->priority;
	sprintf(p->name, "%s", name);
	p->preempt_count = 1; //disable preemtion until schedule_tail

	p->cpu_context.pc = (unsigned long)ret_from_fork;
	p->cpu_context.sp = (unsigned long)childregs;
	int pid = nr_tasks++;
	// task[pid] = p;	
	list_add(&global_all_threads_list, &p->all_threads_list);

	preempt_enable();
	return pid;
}


int move_to_user_mode(unsigned long start, unsigned long size, unsigned long pc)
{
	struct pt_regs *regs = task_pt_regs(current);
	regs->pstate = PSR_MODE_EL0t;
	regs->pc = pc;
	regs->sp = 2 *  PAGE_SIZE;  
	unsigned long code_page = allocate_user_page(current, 0);
	if (code_page == 0)	{
		return -1;
	}
	memcpy(code_page, start, size);
	set_pgd(current->mm.pgd);
	return 0;
}

struct pt_regs *task_pt_regs(TASK_STRUCT *tsk)
{
	unsigned long p = (unsigned long)tsk + THREAD_SIZE - sizeof(struct pt_regs);
	return (struct pt_regs *)p;
}