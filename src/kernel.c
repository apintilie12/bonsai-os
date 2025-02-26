#include "mini_uart.h"
#include "printf.h"
#include "irq.h"
#include "timer.h"

void kernel_main(void) {
    mini_uart_init();
    init_printf(0, putc);
    mini_uart_set_baudrate(9600);
    irq_vector_init();
    timer_init();
    enable_interrupt_controller();
    enable_irq();
    
    while(1) {
        mini_uart_send(mini_uart_recv());
    }
    
}
