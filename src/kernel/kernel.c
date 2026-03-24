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
#include "lib/spinlock.h"
#include "tests/test_ring_buf.h"
#include "tests/test_semaphore.h"
#include "tests/test_string.h"
#include "tests/test_vma.h"
#include "tests/test_bitmap.h"
#include "tests/test_kmalloc.h"
#include "kernel/console.h"
#include "mm/kmalloc.h"
#include "peripherals/emmc.h"

volatile int uart_ready = 0;
volatile int sched_ready = 0;
volatile int init_done = 0;

void kernel_process(){
	// LOG_CORE("Kernel process started. EL %d\r\n", get_el());
	// test_ring_buf();
	// test_semaphore();
	// test_string();
	// test_vma();
	// test_bitmap();
	// test_kmalloc();
	init_done = 1;
	asm volatile("dsb ish" ::: "memory");
	exit_process();
}

void test_spinlock(void) {
	spinlock_t lock = SPINLOCK_INIT;

	LOG_CORE("Testing spinlock...\r\n");

	spin_lock(&lock);
	LOG_CORE("  Lock acquired (Critical Section)\r\n");
	spin_unlock(&lock);

	LOG_CORE("  Lock released. Test Passed!\r\n");
}

static void sd_test(void) {
	unsigned char buf[512];

	if (sd_init() < 0) {
		LOG_CORE("SD init failed\r\n");
		return;
	}
	if (sd_read_block(0, buf) < 0) {
		LOG_CORE("SD read block 0 failed\r\n");
		return;
	}

	LOG_CORE("SD block 0 dump:\r\n");
	for (int i = 0; i < 512; i++) {
		printf("%02x ", buf[i]);
		if ((i & 0xF) == 0xF) printf("\r\n");
	}

	// Quick sanity: MBR signature at bytes 510-511
	if (buf[510] == 0x55 && buf[511] == 0xAA)
		LOG_CORE("MBR signature OK (0x55AA)\r\n");
	else
		LOG_CORE("MBR signature BAD (got 0x%02x%02x)\r\n", buf[510], buf[511]);
}

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
	kmalloc_init();
	LOG_CORE("kmalloc initialized\r\n");

	test_spinlock();

	int res = copy_process(PF_KTHREAD, (unsigned long)&kernel_process, 0, "kernel-thread");
	if (res < 0) {
		LOG_CORE("Error while starting kernel process\r\n");
		return;
	}

	console_init();
	sd_test();
	copy_process(PF_KTHREAD, (unsigned long)&console_task, 0, "console");

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