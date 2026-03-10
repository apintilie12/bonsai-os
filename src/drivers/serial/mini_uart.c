#include "peripherals/mini_uart.h"

#include "peripherals/gpio.h"
#include "arch/utils.h"
#include "lib/printf.h"

void mini_uart_send(char c) {
    while (1) {
        if (get32(AUX_MU_LSR_REG) & 0x20) break;
    }
    put32(AUX_MU_IO_REG, c);
}

char mini_uart_recv(void) {
    while (1) {
        if (get32(AUX_MU_LSR_REG) & 0x01) break;
    }
    return (get32(AUX_MU_IO_REG) & 0xFF);
}

void mini_uart_send_string(char *str) {
    for (int i = 0; str[i] != '\0'; i++) {
        mini_uart_send((char)str[i]);
    }
}

void mini_uart_init(void) {
    unsigned int selector;

    selector = get32(GPFSEL1);
    selector &= ~(7 << 12);  // clean gpio14
    selector |= 2 << 12;     // set alt5 for gpio14
    selector &= ~(7 << 15);  // clean gpio15
    selector |= 2 << 15;     // set alt5 for gpio15
    put32(GPFSEL1, selector);

    /// Removing pull-up/pull-down state from GPIO pins 14 and 15

    put32(GPPUD, 0);
    delay(150);
    put32(GPPUDCLK0, (1 << 14) | (1 << 15));
    delay(150);
    put32(GPPUDCLK0, 0);

    put32(AUX_ENABLES,
          1);  // Enable mini uart (this also enables access to its registers)
    put32(AUX_MU_CNTL_REG, 0);    // Disable auto flow control and disable
                                  // receiver and transmitter (for now)
    // put32(AUX_MU_IER_REG, 0);     // Disable receive and transmit interrupts
    put32(AUX_MU_IER_REG, 1);     // Enable read interrupts and disable transmit interrupts
    put32(AUX_MU_IIR_REG, 2);     // Clear receive FIFO
    put32(AUX_MU_LCR_REG, 3);     // Enable 8 bit mode
    put32(AUX_MU_MCR_REG, 0);     // Set RTS line to be always high
    put32(AUX_MU_BAUD_REG, 270);  // Set baud rate to 115200

    put32(AUX_MU_CNTL_REG, 3);  // Finally, enable transmitter and receiver
}

void mini_uart_set_baudrate(unsigned int baudrate) {
    unsigned int baudrate_reg = (CLOCK_FREQUENCY / (baudrate << 3)) - 1;
    put32(AUX_MU_CNTL_REG, 0);             // Disable RX and TX
    put32(AUX_MU_BAUD_REG, baudrate_reg);  // Set baudrate
    put32(AUX_MU_CNTL_REG, 3);             // Enable RX and TX
}

// This function is required by printf function
void putc ( void* p, char c)
{
	mini_uart_send(c);
}

void mini_uart_handle_irq(void) {
    // char in =(char) get32(AUX_MU_IO_REG) & 0xFF;
    // put32(AUX_MU_IIR_REG, 2);     // Clear receive FIFO
    // printf("MiniUART IRQ triggered by char: %c\r\n", in);
    while (get32(AUX_MU_LSR_REG) & 0x01) {
        char in = (char) get32(AUX_MU_IO_REG) & 0xFF;
        printf("MiniUART IRQ triggered by char: %c\r\n", in);
    }
}