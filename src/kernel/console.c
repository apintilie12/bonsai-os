#include "kernel/console.h"

#include <arch/utils.h>

#include "kernel/reset.h"
#include "drivers/mini_uart.h"
#include "lib/printf.h"
#include "lib/panic.h"
#include "lib/string.h"
#include "lib/ring_buf.h"
#include "kernel/sched.h"
#include "mm/mm.h"

extern volatile int init_done;

#define ARGS_MAX 16

// ----------------------------------------------------------------------------
// Internal utilities
// ----------------------------------------------------------------------------

static void pad_str(const char *s, int width) {
    int len = 0;
    while (s[len]) { mini_uart_send(s[len++]); }
    while (len++ < width) mini_uart_send(' ');
}

static void pad_int(int val, int width) {
    char buf[16];
    int i = 0;
    if (val < 0) { mini_uart_send('-'); val = -val; width--; }
    do { buf[i++] = '0' + (val % 10); val /= 10; } while (val);
    int len = i;
    while (i--) mini_uart_send(buf[i]);
    while (len++ < width) mini_uart_send(' ');
}

static int parse_args(char *line, char *argv[], int max_args) {
    int argc = 0;
    char *p = line;
    while (*p && argc < max_args) {
        while (*p == ' ') p++;
        if (!*p) break;
        argv[argc++] = p;
        while (*p && *p != ' ') p++;
        if (*p) *p++ = '\0';
    }
    return argc;
}

// ----------------------------------------------------------------------------
// Commands
// ----------------------------------------------------------------------------

static void cmd_help(int argc, char *argv[]);
static void cmd_tasks(int argc, char *argv[]);
static void cmd_mem(int argc, char *argv[]);
static void cmd_reboot(int argc, char *argv[]);
static void cmd_panic(int argc, char *argv[]);

typedef struct {
    const char *name;
    void (*fn)(int, char **);
    const char *help;
} cmd_entry_t;

static cmd_entry_t cmd_table[] = {
    { "help",   cmd_help,   "show available commands"        },
    { "tasks",  cmd_tasks,  "list all tasks and their state" },
    { "mem",    cmd_mem,    "show memory usage"              },
    { "reboot", cmd_reboot, "reboot the system"              },
    { "panic",  cmd_panic,  "trigger a test kernel panic"    },
    { 0 }
};

static void cmd_help(int argc, char *argv[]) {
    for (cmd_entry_t *e = cmd_table; e->name; e++) {
        printf("  ");
        pad_str(e->name, 10);
        printf("%s\r\n", e->help);
    }
}

static void cmd_tasks(int argc, char *argv[]) {
    TASK_STRUCT *p;
    printf("  "); pad_str("name", 16); pad_str("state", 8); pad_str("cpu", 6); printf("counter\r\n");
    printf("  "); pad_str("----", 16); pad_str("-----", 8); pad_str("---", 6); printf("-------\r\n");
    LIST_FOR_EACH_ENTRY(p, &global_all_threads_list, all_threads_list) {
        const char *state = p->state == TASK_RUNNING ? "running" : "zombie";
        printf("  ");
        pad_str(p->name, 16);
        pad_str(state, 8);
        pad_int(p->on_cpu, 6);
        pad_int((int)p->counter, 8);
        printf("\r\n");
    }
}

static void cmd_panic(int argc, char *argv[]) {
    panic("test panic triggered from console");
}

static void cmd_reboot(int argc, char *argv[]) {
    printf("Rebooting...\r\n");
    delay(50000000);
    reboot();
}

static void cmd_mem(int argc, char *argv[]) {
    unsigned long total, used;
    get_mem_stats(&total, &used);
    unsigned long free = total - used;
    printf("  Page size : %d B\r\n", PAGE_SIZE);
    printf("  Total     : "); pad_int(total, 0); printf(" pages ("); pad_int((total * PAGE_SIZE) / 1024, 0); printf(" KB)\r\n");
    printf("  Used      : "); pad_int(used,  0); printf(" pages ("); pad_int((used  * PAGE_SIZE) / 1024, 0); printf(" KB)\r\n");
    printf("  Free      : "); pad_int(free,  0); printf(" pages ("); pad_int((free  * PAGE_SIZE) / 1024, 0); printf(" KB)\r\n");
}

// ----------------------------------------------------------------------------
// History
// ----------------------------------------------------------------------------

static char _history_storage[CONSOLE_HISTORY_SIZE][CONSOLE_LINE_MAX];
static ring_buf_t history = RING_BUF_INIT(_history_storage, CONSOLE_HISTORY_SIZE, CONSOLE_LINE_MAX);

static void history_push(const char *line) {
    char discard[CONSOLE_LINE_MAX];
    if (ring_buf_is_full(&history))
        ring_buf_pop(&history, discard); // drop oldest to make room
    ring_buf_push(&history, line);
}

// offset 0 = most recent, 1 = one before, etc.
static int history_get(unsigned int offset, char *out) {
    return ring_buf_peek(&history, offset, out);
}

// ----------------------------------------------------------------------------
// Line editor
// ----------------------------------------------------------------------------

static void line_clear(int len) {
    for (int i = 0; i < len; i++) mini_uart_send('\b');
    for (int i = 0; i < len; i++) mini_uart_send(' ');
    for (int i = 0; i < len; i++) mini_uart_send('\b');
}

void console_readline(char *buf, int max) {
    int len = 0;
    int hist_pos = 0;            // 0 = live input; 1+ = history offset
    char saved[CONSOLE_LINE_MAX]; // live input saved while browsing history
    saved[0] = '\0';
    char c;

    while (1) {
        uart_rx_wait();
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

        if (c == 0x1B) { // ESC: start of arrow key sequence
            char seq1, seq2;
            uart_rx_wait(); uart_buf_pop(&seq1);
            uart_rx_wait(); uart_buf_pop(&seq2);

            if (seq1 != '[') continue;
            if (seq2 != 'A' && seq2 != 'B') continue; // only handle up/down

            int new_pos = hist_pos + (seq2 == 'A' ? 1 : -1);
            if (new_pos < 0) new_pos = 0;
            unsigned int count = ring_buf_count(&history);
            if ((unsigned int)new_pos > count) new_pos = (int)count;
            if (new_pos == hist_pos) continue;

            // save live buffer before starting to browse
            if (hist_pos == 0) {
                buf[len] = '\0';
                for (int i = 0; i <= len; i++) saved[i] = buf[i];
            }

            line_clear(len);
            hist_pos = new_pos;

            if (hist_pos == 0) {
                for (len = 0; saved[len]; len++) buf[len] = saved[len];
            } else {
                len = 0;
                if (history_get((unsigned int)(hist_pos - 1), buf)) {
                    while (buf[len]) len++;
                }
            }
            buf[len] = '\0';
            for (int i = 0; i < len; i++) mini_uart_send(buf[i]);
            continue;
        }

        if (c >= 32 && len < max - 1) {
            buf[len++] = c;
            mini_uart_send(c);
            hist_pos = 0; // typing cancels history browsing
        }
    }

    buf[len] = '\0';
}

// ----------------------------------------------------------------------------
// Console task
// ----------------------------------------------------------------------------

void console_task(void) {
    char line[CONSOLE_LINE_MAX];
    char *argv[ARGS_MAX];

    while (!init_done) {}

    printf("\r\n\033[32mBonsai OS\033[0m Console\r\n");
    printf("Type 'help' for available commands.\r\n\r\n");

    while (1) {
        printf("\033[32m>\033[0m ");
        console_readline(line, CONSOLE_LINE_MAX);

        if (line[0] == '\0')
            continue;

        history_push(line);

        int argc = parse_args(line, argv, ARGS_MAX);
        if (argc == 0)
            continue;

        int found = 0;
        for (cmd_entry_t *e = cmd_table; e->name; e++) {
            if (strcmp(argv[0], e->name) == 0) {
                e->fn(argc, argv);
                found = 1;
                break;
            }
        }
        if (!found)
            printf("Unknown command: '%s'. Type 'help' for available commands.\r\n", argv[0]);
    }
}
