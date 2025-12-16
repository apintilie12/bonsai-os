#include <stddef.h>
#include <stdint.h>

#include "printf.h"
#include "utils.h"
#include "timer.h"
#include "irq.h"
#include "fork.h"
#include "sched.h"
#include "mini_uart.h"
#include "sys.h"
#include "user.h"
#include "spinlock.h"


void kernel_process(){
	printf("Kernel process started. EL %d\r\n", get_el());
	unsigned long begin = (unsigned long)&user_begin;
	unsigned long end = (unsigned long)&user_end;
	unsigned long process = (unsigned long)&user_process;
	int err = move_to_user_mode(begin, end - begin, process - begin);
	if (err < 0){
		printf("Error while moving process to user mode\n\r");
	} 
}

void test_spinlock(void) {
	spinlock_t lock = SPINLOCK_INIT;
	
	printf("Testing spinlock...\r\n");
	
	spin_lock(&lock);
	printf("  Lock acquired (Critical Section)\r\n");
	spin_unlock(&lock);
	
	printf("  Lock released. Test Passed!\r\n");
}


void kernel_main()
{
	mini_uart_init();
	init_printf(NULL, putc);
	irq_vector_init();
	timer_init();
	sched_init();
	enable_interrupt_controller();
	enable_irq();

	test_spinlock();

	int res = copy_process(PF_KTHREAD, (unsigned long)&kernel_process, 0, "kernel-thread");
	if (res < 0) {
		printf("error while starting kernel process");
		return;
	}

	CPU_INFO * cpu_info = get_cpu_info();
	printf("Core %d started\r\n", cpu_info->cpu_id);

	while (1){
		schedule();
	}	
}
