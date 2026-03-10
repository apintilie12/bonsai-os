#include "peripherals/pl_uart.h"

#include "peripherals/gpio.h"
#include "arch/utils.h"

void pl_uart_init(void) {
    unsigned int selector;

    selector = get32(GPFSEL3);
    selector &= ~(7 << 6);  // Clean gpio32
    selector |= 7 << 6;     // Set alt3 for gpio32
    selector &= ~(7 << 9);  // Clean gpio33
    selector |= 7 << 9;     // Set alt3 for gpio33
    put32(GPFSEL3, selector);

    /// Remove pull-up/pull-down state from GPIO pins 32 and 33

    put32(GPPUD, 0);
    delay(150);
    put32(GPPUDCLK1, 3);
    delay(150);
    put32(GPPUDCLK1, 0);

    /// UART setup

    put32(UART_CR, 0);           /// Disable UART
    // put32(UART_LCRH, 0x70);  /// Set 8 bit word length
    put32(UART_LCRH, (3 << 5));
    /// Set baudrate 115200              ///Bauddiv = 136.5
    put32(UART_IBRD, UART_IBRD_VAL);  
    put32(UART_FBRD, UART_FBRD_VAL);   

    put32(UART_CR, (3 << 8) | 1);  /// Enable UART RX and TX
}

void pl_uart_send(char c) {
    while (1) {
        if (!(get32(UART_FR) & 0x20) ) {
            break;
        }
    }
    put32(UART_DR, c);
}

char pl_uart_recv(void) {
    while (1) {
        if (!(get32(UART_FR) &0x10)) {
            break;
        }
    }
    return (get32(UART_DR) & 0xFF);
}

void pl_uart_send_string(char *str) {
    for (int i = 0; str[i] != '\0'; i++) {
        pl_uart_send((char)str[i]);
    }
}