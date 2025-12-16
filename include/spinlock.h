#ifndef _SPINLOCK_H
#define _SPINLOCK_H

typedef struct {
	volatile int lock;
} spinlock_t;

#define SPINLOCK_INIT {0}

void spin_lock(spinlock_t *lock);
void spin_unlock(spinlock_t *lock);

#endif