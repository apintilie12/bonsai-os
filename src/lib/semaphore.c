#include "lib/semaphore.h"
#include "kernel/sched.h"

void sem_init(semaphore_t *sem, int count) {
    sem->count = count;
    list_head_init(&sem->waiters);
    sem->lock = (spinlock_t)SPINLOCK_INIT;
}

void sem_wait(semaphore_t *sem) {
    sem_waiter_t node;
    node.task = get_current_task();
    int in_list = 0;

    while (1) {
        spin_lock(&sem->lock);

        if (sem->count > 0) {
            sem->count--;
            if (in_list)
                list_del(&node.entry);
            spin_unlock(&sem->lock);
            return;
        }

        // Add to waiters before setting TASK_WAITING — both under sem->lock.
        // sem_signal also holds sem->lock when it checks waiters and sets
        // TASK_RUNNING, so there is no window where a signal can be missed.
        if (!in_list) {
            list_add_tail(&sem->waiters, &node.entry);
            in_list = 1;
        }
        get_current_task()->state = TASK_WAITING;
        spin_unlock(&sem->lock);

        schedule(); // yields; returns when state is TASK_RUNNING again
    }
}

void sem_signal(semaphore_t *sem) {
    spin_lock(&sem->lock);
    sem->count++;
    if (!list_empty(&sem->waiters)) {
        sem_waiter_t *w = CONTAINER_OF(sem->waiters.next, sem_waiter_t, entry);
        // Set RUNNING without removing from list — the waiter removes itself
        // in sem_wait once it successfully decrements count.
        w->task->state = TASK_RUNNING;
        asm volatile("dsb ish" ::: "memory");
    }
    spin_unlock(&sem->lock);
}
