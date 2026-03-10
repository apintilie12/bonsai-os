#ifndef _LOG_H
#define _LOG_H

#include "lib/printf.h"

#define _BASENAME \
    (__builtin_strrchr(__FILE__, '/') ? __builtin_strrchr(__FILE__, '/') + 1 : __FILE__)

#define _CPU_ID() \
    ({ unsigned long _id; asm volatile("mrs %0, mpidr_el1" : "=r"(_id)); (int)(_id & 0xFF); })

#define LOG(fmt, ...) \
    printf(fmt, ##__VA_ARGS__)

#define LOG_CORE(fmt, ...) \
    printf("[Core %d]: " fmt, _CPU_ID(), ##__VA_ARGS__)

#define LOG_DEBUG(fmt, ...) \
    printf("[Core %d | %s:%d]: " fmt, _CPU_ID(), _BASENAME, __LINE__, ##__VA_ARGS__)

#define LOG_TRACE(fmt, ...) \
    printf("[Core %d | %s() | %s:%d]: " fmt, _CPU_ID(), __func__, _BASENAME, __LINE__, ##__VA_ARGS__)

#endif
