#include <stddef.h>
#include <stdint.h>

#include "lib/log.h"
#include "arch/utils.h"
#include "drivers/timer.h"
#include "kernel/irq.h"
#include "kernel/fork.h"
#include "kernel/sched.h"
#include "drivers/mini_uart.h"
#include "kernel/sys.h"
#include "kernel/user.h"
#include "lib/spinlock.h"
#include "tests/test_ring_buf.h"

#define DEMO_DELAY 50000000

void demo_task_a(void) {
	while (1) {
		LOG_CORE("demo-a running\r\n");
		delay(DEMO_DELAY);
	}
}

void demo_task_b(void) {
	while (1) {
		LOG_CORE("demo-b running\r\n");
		delay(DEMO_DELAY);
	}
}

void demo_task_c(void) {
	while (1) {
		LOG_CORE("demo-c running\r\n");
		delay(DEMO_DELAY);
	}
}

void demo_task_d(void) {
	while (1) {
		LOG_CORE("demo-d running\r\n");
		delay(DEMO_DELAY);
	}
}

void kernel_process(){
	LOG_CORE("Kernel process started. EL %d\r\n", get_el());
	// unsigned long begin = (unsigned long)&user_begin;
	// unsigned long end = (unsigned long)&user_end;
	// unsigned long process = (unsigned long)&user_process;
	// int err = move_to_user_mode(begin, end - begin, process - begin);
	// if (err < 0){
	// 	LOG_CORE("Error while moving process to user mode\r\n");
	// }
	test_ring_buf();
	while (1) {
	}
}

void test_spinlock(void) {
	spinlock_t lock = SPINLOCK_INIT;

	LOG_CORE("Testing spinlock...\r\n");

	spin_lock(&lock);
	LOG_CORE("  Lock acquired (Critical Section)\r\n");
	spin_unlock(&lock);

	LOG_CORE("  Lock released. Test Passed!\r\n");
}


volatile int uart_ready = 0;
volatile int sched_ready = 0;

void kernel_main()
{
	mini_uart_init();
	init_printf(NULL, putc);
	uart_ready = 1;
	asm volatile("dsb ish" ::: "memory");
	LOG_CORE("UART enabled\r\n");
	irq_vector_init();
	LOG_CORE("IRQ vector initialized\r\n");
	timer_init();
	LOG_CORE("Timer initialized\r\n");
	sched_init();
	sched_ready = 1;
	asm volatile("dsb ish" ::: "memory");
	LOG_CORE("Scheduler initialized\r\n");
	enable_interrupt_controller();
	LOG_CORE("Interrupt controller enabled\r\n");
	enable_irq();
	LOG_CORE("Interrupts enabled\r\n");

	test_spinlock();

	int res = copy_process(PF_KTHREAD, (unsigned long)&kernel_process, 0, "kernel-thread");
	if (res < 0) {
		LOG_CORE("Error while starting kernel process\r\n");
		return;
	}

	copy_process(PF_KTHREAD, (unsigned long)&demo_task_a, 0, "demo-a");
	copy_process(PF_KTHREAD, (unsigned long)&demo_task_b, 0, "demo-b");
	copy_process(PF_KTHREAD, (unsigned long)&demo_task_c, 0, "demo-c");
	copy_process(PF_KTHREAD, (unsigned long)&demo_task_d, 0, "demo-d");

	LOG_CORE("Entering idle loop\r\n");

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
	while (!sched_ready) {
	}

	sched_init_secondary(cpu_id);

	timer_init();
	enable_core_timer_irq(cpu_id);
	enable_irq();

	LOG_CORE("Entering idle loop\r\n");
	while (1) {
		schedule();
	}
}