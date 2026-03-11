#ifndef _SPINLOCK_H
#define _SPINLOCK_H

typedef struct {
	volatile int lock;
} spinlock_t;

#define SPINLOCK_INIT {0}

void spin_lock(spinlock_t *lock);
void spin_unlock(spinlock_t *lock);

// Use these variants for locks that may be acquired from both task and IRQ context.
// They save/restore DAIF so IRQs are disabled while the lock is held and restored
// to their previous state (enabled or disabled) on release.
static inline void spin_lock_irqsave(spinlock_t *lock, unsigned long *flags) {
	asm volatile("mrs %0, daif" : "=r"(*flags) :: "memory");
	asm volatile("msr daifset, #2" ::: "memory");
	spin_lock(lock);
}

static inline void spin_unlock_irqrestore(spinlock_t *lock, unsigned long flags) {
	spin_unlock(lock);
	asm volatile("msr daif, %0" :: "r"(flags) : "memory");
}

#endif