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
        unsigned long flags;
        spin_lock_irqsave(&sem->lock, &flags);

        if (sem->count > 0) {
            sem->count--;
            if (in_list)
                list_del(&node.entry);
            spin_unlock_irqrestore(&sem->lock, flags);
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
        spin_unlock_irqrestore(&sem->lock, flags);

        schedule(); // yields; returns when state is TASK_RUNNING again
    }
}

void sem_signal(semaphore_t *sem) {
    unsigned long flags;
    spin_lock_irqsave(&sem->lock, &flags);
    sem->count++;
    // Find the first waiter that is still TASK_WAITING and wake it.
    // Waiters already set to TASK_RUNNING (woken by a prior signal but not
    // yet scheduled) are skipped so rapid back-to-back signals each wake a
    // distinct waiter rather than repeatedly signalling the same one.
    sem_waiter_t *w;
    LIST_FOR_EACH_ENTRY(w, &sem->waiters, entry) {
        if (w->task->state == TASK_WAITING) {
            w->task->state = TASK_RUNNING;
            asm volatile("dsb ish" ::: "memory");
            break;
        }
    }
    spin_unlock_irqrestore(&sem->lock, flags);
}
