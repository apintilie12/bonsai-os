#ifndef _SEMAPHORE_H
#define _SEMAPHORE_H

#include "lib/list.h"
#include "lib/spinlock.h"

// Forward declaration — full definition in sched.h
typedef struct _TASK_STRUCT TASK_STRUCT;

typedef struct {
    TASK_STRUCT *task;
    LIST_ENTRY   entry;
} sem_waiter_t;

typedef struct {
    int        count;
    LIST_ENTRY waiters;
    spinlock_t lock;
} semaphore_t;

void sem_init(semaphore_t *sem, int count);
void sem_wait(semaphore_t *sem);
void sem_signal(semaphore_t *sem);

#endif /* _SEMAPHORE_H */
