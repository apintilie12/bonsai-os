#ifndef _CONSOLE_H
#define _CONSOLE_H

#define CONSOLE_LINE_MAX 128
#define CONSOLE_HISTORY_SIZE 16

void console_readline(char *buf, int max);
void console_task(void);

#endif
