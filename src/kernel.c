#include "mini_uart.h"
#include "printf.h"
#include "utils.h"

void kernel_main(void) {
    mini_uart_init();
    init_printf(0, putc);
    mini_uart_set_baudrate(9600);
    int el = get_el();
    printf("Exception level: %d \r\n", el);
    while(1) {
        mini_uart_send(mini_uart_recv());
    }
    
}
