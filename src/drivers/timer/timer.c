#include "arch/utils.h"
#include "lib/log.h"
#include "kernel/sched.h"
#include "kernel/irq.h"

#define HZ 100
#define CNTP_CTL_ENABLE 1

static unsigned int interval = 0;

void handle_timer_irq(void)
{
	write_cntp_tval(interval);
	timer_tick();
}

void timer_init(void)
{
	unsigned long frq = get_cntfrq();
	
	// Sanity check: If uninitialized, default to standard RPi frequency (19.2MHz)
	if (frq == 0) {
		frq = 19200000;
	}
	LOG_CORE("Timer frequency: %u Hz\r\n", frq);

	interval = frq / HZ;
	write_cntp_tval(interval);
	write_cntp_ctl(CNTP_CTL_ENABLE);
	irq_register_local(0x2, handle_timer_irq, "timer");
}
