#ifndef _PL_UART
#define _PL_UART

void pl_uart_init( void );
void pl_uart_send( char c );
char pl_uart_recv( void );
void pl_uart_send_string( char* str );

#endif /*_PL_UART*/