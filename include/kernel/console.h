#ifndef _CONSOLE_H
#define _CONSOLE_H

#define CONSOLE_LINE_MAX    128
#define CONSOLE_HISTORY_SIZE 16
#define CONSOLE_CMD_MAX      32

// Register a command. Call before console_task is scheduled.
void console_register_cmd(const char *name, void (*fn)(int, char **), const char *help);

// Register all built-in commands. Call from kernel_main before forking console_task.
void console_init(void);

void console_readline(char *buf, int max);
void console_task(void);

#endif
