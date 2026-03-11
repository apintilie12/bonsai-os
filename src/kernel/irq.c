#include "kernel/irq.h"
#include "arch/utils.h"
#include "lib/printf.h"
#include "lib/panic.h"
#include "lib/spinlock.h"
#include "arch/entry.h"
#include "peripherals/irq.h"
#include "peripherals/qa7.h"

// ----------------------------------------------------------------------------
// Invalid exception messages
// ----------------------------------------------------------------------------

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

void show_invalid_entry_message(int type, unsigned long esr, unsigned long address)
{
	panic("%s, ESR: %x, address: %x", entry_error_messages[type], esr, address);
}

// ----------------------------------------------------------------------------
// IRQ handler registration
// ----------------------------------------------------------------------------

#define IRQ_TABLE_MAX 8

typedef struct {
	unsigned int   bit;
	void         (*handler)(void);
	const char    *name;
} irq_entry_t;

static irq_entry_t local_table[IRQ_TABLE_MAX];
static int         local_count = 0;

static irq_entry_t gpu_table[IRQ_TABLE_MAX];
static int         gpu_count = 0;

static spinlock_t irq_reg_lock = SPINLOCK_INIT;

void irq_register_local(unsigned int bit, void (*handler)(void), const char *name) {
	spin_lock(&irq_reg_lock);
	for (int i = 0; i < local_count; i++) {
		if (local_table[i].bit == bit) {
			spin_unlock(&irq_reg_lock);
			return; // already registered, skip (e.g. timer called from each core)
		}
	}
	if (local_count >= IRQ_TABLE_MAX)
		panic("irq_register_local: table full");
	local_table[local_count++] = (irq_entry_t){ bit, handler, name };
	spin_unlock(&irq_reg_lock);
}

void irq_register_gpu(unsigned int bit, void (*handler)(void), const char *name) {
	spin_lock(&irq_reg_lock);
	for (int i = 0; i < gpu_count; i++) {
		if (gpu_table[i].bit == bit) {
			spin_unlock(&irq_reg_lock);
			return;
		}
	}
	if (gpu_count >= IRQ_TABLE_MAX)
		panic("irq_register_gpu: table full");
	gpu_table[gpu_count++] = (irq_entry_t){ bit, handler, name };
	spin_unlock(&irq_reg_lock);
}

void irq_print_handlers(void) {
	printf("  LOCAL (QA7):\r\n");
	if (local_count == 0)
		printf("    (none)\r\n");
	for (int i = 0; i < local_count; i++)
		printf("    bit 0x%x   %s\r\n", local_table[i].bit, local_table[i].name);

	printf("  GPU (PENDING_1):\r\n");
	if (gpu_count == 0)
		printf("    (none)\r\n");
	for (int i = 0; i < gpu_count; i++)
		printf("    bit 0x%x   %s\r\n", gpu_table[i].bit, gpu_table[i].name);
}

// ----------------------------------------------------------------------------
// Hardware enable
// ----------------------------------------------------------------------------

void enable_core_timer_irq(int cpu_id)
{
	// Bit 1: nCNTPNSIRQ (Non-Secure Physical Timer)
	put32(CORE0_TIMER_IRQCNTL + (cpu_id * 4), 0x2);
}

void enable_interrupt_controller()
{
	// Enable Mini UART (Legacy IRQ, routed to core 0 only)
	put32(ENABLE_IRQS_1, AUX_INT_IRQ);
	enable_core_timer_irq(0);
}

// ----------------------------------------------------------------------------
// IRQ dispatch
// ----------------------------------------------------------------------------

void handle_irq(void)
{
	unsigned long mpidr;
	asm volatile("mrs %0, mpidr_el1" : "=r"(mpidr));
	unsigned int src = get32(CORE0_IRQ_SOURCE + ((mpidr & 0xFF) * 4));

	for (int i = 0; i < local_count; i++) {
		if (src & local_table[i].bit)
			local_table[i].handler();
	}

	if (src & 0x100) {
		unsigned int pending = get32(IRQ_PENDING_1);
		int handled = 0;
		for (int i = 0; i < gpu_count; i++) {
			if (pending & gpu_table[i].bit) {
				gpu_table[i].handler();
				handled = 1;
			}
		}
		if (!handled)
			printf("handle_irq: unknown GPU IRQ, PENDING_1=0x%x\r\n", pending);
	}
}
