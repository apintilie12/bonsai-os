#include "mini_uart.h"
#include "pl_uart.h"

void kernel_main(void) {
    // mini_uart_init();
    // mini_uart_set_baudrate(9600);
    // mini_uart_send_string("Sending at 9600 baudrate!\r\n");

    // while(1) {
    //     mini_uart_send(mini_uart_recv());
    // }
    pl_uart_init();
    pl_uart_send_string("Hello from the main UART!\r\n");

    while (1) {
        pl_uart_send(pl_uart_recv());
    }
}
