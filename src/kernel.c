#include "mini_uart.h"
#include "printf.h"

void kernel_main(void) {
    mini_uart_init();
    init_printf(0, putc);
    mini_uart_set_baudrate(9600);
    mini_uart_send_string("Sending at 9600 baudrate!\r\n");
    for(int i = 0; i < 10; i++) {
        printf("I at value: %d\r\n", i);
    }

    while(1) {
        mini_uart_send(mini_uart_recv());
    }
    
}
