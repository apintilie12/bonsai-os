#include "utils.h"
#include "printf.h"
#include "timer.h"
#include "entry.h"
#include "peripherals/irq.h"
#include "peripherals/qa7.h"
#include "mini_uart.h"

const char *entry_error_messages[] = {
	"SYNC_INVALID_EL1t",
	"IRQ_INVALID_EL1t",		
	"FIQ_INVALID_EL1t",		
	"ERROR_INVALID_EL1T",		

	"SYNC_INVALID_EL1h",		
	"IRQ_INVALID_EL1h",		
	"FIQ_INVALID_EL1h",		
	"ERROR_INVALID_EL1h",		

	"SYNC_INVALID_EL0_64",		
	"IRQ_INVALID_EL0_64",		
	"FIQ_INVALID_EL0_64",		
	"ERROR_INVALID_EL0_64",	

	"SYNC_INVALID_EL0_32",		
	"IRQ_INVALID_EL0_32",		
	"FIQ_INVALID_EL0_32",		
	"ERROR_INVALID_EL0_32"	
};

void enable_interrupt_controller()
{
	// Enable Mini UART (Legacy)
    put32(ENABLE_IRQS_1, AUX_INT_IRQ);

	// Enable Core 0 Timer (QA7) - Bit 1: nCNTPNSIRQ (Non-Secure Physical Timer)
	put32(CORE0_TIMER_IRQCNTL, 0x2);
}

void show_invalid_entry_message(int type, unsigned long esr, unsigned long address)
{
	unsigned long mpidr;
	asm volatile("mrs %0, mpidr_el1" : "=r"(mpidr));
	int cpu_id = mpidr & 0xFF;
	printf("Core %d: %s, ESR: %x, address: %x\r\n", cpu_id, entry_error_messages[type], esr, address);
}

void handle_irq(void)
{
	unsigned int src = get32(CORE0_IRQ_SOURCE);

	// Check Local Timer Interrupt (Bit 1: nCNTPNSIRQ)
	if (src & 0x2) {
		handle_timer_irq();
	}
	// Check Legacy GPU Interrupts (Bit 8)
	if (src & 0x100) {
		unsigned int irq = get32(IRQ_PENDING_1);
		if (irq & AUX_INT_IRQ) {
			mini_uart_handle_irq();
		} else {
			printf("Unknown pending irq: %x\r\n", irq);
		}
	}
}