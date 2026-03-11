#ifndef	_IRQ_H
#define	_IRQ_H

void enable_interrupt_controller(void);
void enable_core_timer_irq(int cpu_id);

void irq_vector_init(void);
void enable_irq(void);
void disable_irq(void);

// Register a handler for a QA7 local IRQ source bit (e.g. 0x2 = timer).
// Safe to call from multiple cores — duplicate bits are silently ignored.
void irq_register_local(unsigned int bit, void (*handler)(void), const char *name);

// Register a handler for a GPU legacy IRQ (IRQ_PENDING_1 bit, core 0 only).
void irq_register_gpu(unsigned int bit, void (*handler)(void), const char *name);

// Print all registered handlers via printf (for the 'irqs' console command).
void irq_print_handlers(void);

#endif  /*_IRQ_H */