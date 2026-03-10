#include "kernel/console.h"
#include "drivers/mini_uart.h"

void console_readline(char *buf, int max) {
    int len = 0;
    char c;

    while (1) {
        if (!uart_buf_pop(&c))
            continue;

        if (c == '\r' || c == '\n') {
            mini_uart_send('\r');
            mini_uart_send('\n');
            break;
        }

        if ((c == '\b' || c == 127) && len > 0) {
            len--;
            mini_uart_send('\b');
            mini_uart_send(' ');
            mini_uart_send('\b');
            continue;
        }

        if (c >= 32 && len < max - 1) {
            buf[len++] = c;
            mini_uart_send(c);
        }
    }

    buf[len] = '\0';
}
