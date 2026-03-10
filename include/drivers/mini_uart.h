#ifndef	_MINI_UART_H
#define	_MINI_UART_H

void mini_uart_init ( void );
char mini_uart_recv ( void );
void mini_uart_send ( char c );
void mini_uart_send_string(char* str);
void mini_uart_set_baudrate(unsigned int baudrate);
void putc ( void* p, char c );
void mini_uart_handle_irq(void);
int  uart_buf_pop(char *out);
void uart_rx_wait(void);
#endif  /*_MINI_UART_H */
