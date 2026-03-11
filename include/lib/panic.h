#ifndef _PANIC_H
#define _PANIC_H

void panic(const char *fmt, ...);

#define BUG_ON(cond) \
    do { if (cond) panic("BUG at %s:%d: " #cond, __FILE__, __LINE__); } while (0)

#define ASSERT(cond) BUG_ON(!(cond))

#endif
