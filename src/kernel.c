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


volatile int uart_ready = 0;

void kernel_main()
{
	mini_uart_init();
	init_printf(NULL, putc);
	uart_ready = 1;
	asm volatile("dsb ish" ::: "memory");
	printf("UART enabled\r\n");
	irq_vector_init();
	printf("IRQ vector initialized\r\n");
	timer_init();
	printf("Timer initialized\r\n");
	sched_init();
	printf("Scheduler initialized\r\n");
	enable_interrupt_controller();
	printf("Interrupt controller enabled\r\n");
	enable_irq();
	printf("Interrupts enabled\r\n");


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

void secondary_start(void) {
	mini_uart_send('S'); // Probe A: reached secondary_start

	// Get CPU ID directly from the register, before tpidr_el1 is valid
	unsigned long cpu_id;
	asm volatile("mrs %0, mpidr_el1" : "=r"(cpu_id));
	cpu_id &= 0xFF;

	cpu_data[cpu_id].cpu_id = cpu_id;
	asm volatile("msr tpidr_el1, %0" :: "r"(&cpu_data[cpu_id]));
	irq_vector_init();

	mini_uart_send('W'); // Probe B: about to enter uart_ready loop
	while (!uart_ready) {
	}
	mini_uart_send('G'); // Probe C: exited uart_ready loop

	CPU_INFO * cpu_info = get_cpu_info();
	printf("Core %d started\r\n", cpu_info->cpu_id);
	while (1) {

	}
}