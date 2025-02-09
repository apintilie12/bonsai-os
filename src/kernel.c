#include "mini_uart.h"

void kernel_main(void) {
    mini_uart_init();
    mini_uart_set_baudrate(115200);
    mini_uart_send_string("Sending at 9600 baudrate!\r\n");

    while(1) {
        mini_uart_send(mini_uart_recv());
    }
    
}
