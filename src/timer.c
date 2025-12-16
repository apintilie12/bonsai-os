#include "utils.h"
#include "printf.h"
#include "sched.h"

#define HZ 100
#define CNTP_CTL_ENABLE 1

static unsigned int interval = 0;

void timer_init(void)
{
	unsigned long frq = get_cntfrq();
	
	// Sanity check: If uninitialized, default to standard RPi frequency (19.2MHz)
	if (frq == 0) {
		frq = 19200000;
	}
	printf("Timer frequency: %u Hz\r\n", frq);

	interval = frq / HZ;
	write_cntp_tval(interval);
	write_cntp_ctl(CNTP_CTL_ENABLE);
}

void handle_timer_irq(void) 
{
	write_cntp_tval(interval);
	timer_tick();
}