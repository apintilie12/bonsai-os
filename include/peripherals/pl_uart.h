#ifndef _P_PL_UART_H
#define _P_PL_UART_H

#include "peripherals/base.h"

#define UART_REG_BASE_ADDR	(PBASE+0x00201000)

#define UART_DR         (UART_REG_BASE_ADDR + 0x00)
#define UART_FR         (UART_REG_BASE_ADDR + 0x18)
#define UART_IBRD       (UART_REG_BASE_ADDR + 0x24)
#define UART_FBRD       (UART_REG_BASE_ADDR + 0x28)
#define UART_LCRH       (UART_REG_BASE_ADDR + 0x2C)
#define UART_CR         (UART_REG_BASE_ADDR + 0x30)
#define UART_IMSC       (UART_REG_BASE_ADDR + 0x38)

#define MHz             1000000
#define UART_CLK        (48 * MHz)
#define UART_BAUD_RATE  115200
#define UART_BAUD_DIV   (((double)UART_CLK) / (16 * UART_BAUD_RATE))
#define UART_IBRD_VAL   ((unsigned int)(UART_BAUD_DIV))
#define UART_FBRD_VAL   ((unsigned int)((UART_BAUD_DIV - UART_IBRD_VAL) * 64 + .5))

#endif /*_P_PL_UART_H*/
