#include "mini_uart.h"
#include "string.h"
#include "utils.h"

void kernel_main(unsigned int cpuid) {
    if(cpuid == 0) {
        mini_uart_init();
        mini_uart_set_baudrate(9600);
        mini_uart_send_string("Main core started at least...\r\n");
    }
    long delay_size = 1000000 * cpuid;
    char buf[5];
    itoa(cpuid, buf);
    static char msg[100] = "Hello from core: ";
    // mini_uart_send_string("got here 1\r\n");
    strcat(msg, buf);
    // mini_uart_send_string("got here 2\r\n");
    strcat(msg, "\r\n\0");
    // mini_uart_send_string("got here 3\r\n");
    delay(delay_size);
    mini_uart_send_string(msg);

    while(1) {
        // mini_uart_send(mini_uart_recv());
    }
    
}
