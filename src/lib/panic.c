#include "lib/panic.h"
#include "lib/printf.h"
#include "drivers/mini_uart.h"
#include "kernel/irq.h"

#include <stdarg.h>

void panic(const char *fmt, ...) {
    unsigned long cpu_id;
    asm volatile("mrs %0, mpidr_el1" : "=r"(cpu_id));
    cpu_id &= 0xFF;

    disable_irq();

    // Use tfp_format + putc directly to bypass the printf spinlock — panic may
    // be called from any context, including while the printf lock is held.
    mini_uart_send_string("\r\n\033[31mKERNEL PANIC [core ");
    mini_uart_send('0' + (char)cpu_id);
    mini_uart_send_string("]: ");

    va_list va;
    va_start(va, fmt);
    tfp_format(0, putc, (char *)fmt, va);
    va_end(va);

    mini_uart_send_string("\033[0m\r\n");

    while (1) {
        asm volatile("wfe");
    }
}
