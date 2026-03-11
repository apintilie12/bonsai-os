#include "lib/ring_buf.h"

static void rb_memcpy(void *dst, const void *src, unsigned int n) {
    unsigned char *d = (unsigned char *)dst;
    const unsigned char *s = (const unsigned char *)src;
    while (n--) *d++ = *s++;
}

int ring_buf_push(ring_buf_t *rb, const void *elem) {
    unsigned long flags;
    spin_lock_irqsave(&rb->lock, &flags);
    unsigned int next = (rb->head + 1) % rb->capacity;
    if (next == rb->tail) {
        spin_unlock_irqrestore(&rb->lock, flags);
        return 0;  // full, drop
    }
    unsigned char *slot = (unsigned char *)rb->buf + rb->head * rb->elem_size;
    rb_memcpy(slot, elem, rb->elem_size);
    rb->head = next;
    spin_unlock_irqrestore(&rb->lock, flags);
    return 1;
}

int ring_buf_pop(ring_buf_t *rb, void *out) {
    unsigned long flags;
    spin_lock_irqsave(&rb->lock, &flags);
    if (rb->tail == rb->head) {
        spin_unlock_irqrestore(&rb->lock, flags);
        return 0;  // empty
    }
    unsigned char *slot = (unsigned char *)rb->buf + rb->tail * rb->elem_size;
    rb_memcpy(out, slot, rb->elem_size);
    rb->tail = (rb->tail + 1) % rb->capacity;
    spin_unlock_irqrestore(&rb->lock, flags);
    return 1;
}

// offset 0 = most recently pushed, 1 = one before that, etc.
int ring_buf_peek(ring_buf_t *rb, unsigned int offset, void *out) {
    unsigned long flags;
    spin_lock_irqsave(&rb->lock, &flags);
    unsigned int count = (rb->head - rb->tail + rb->capacity) % rb->capacity;
    if (offset >= count) {
        spin_unlock_irqrestore(&rb->lock, flags);
        return 0;
    }
    unsigned int idx = (rb->head - 1 - offset + rb->capacity) % rb->capacity;
    unsigned char *slot = (unsigned char *)rb->buf + idx * rb->elem_size;
    rb_memcpy(out, slot, rb->elem_size);
    spin_unlock_irqrestore(&rb->lock, flags);
    return 1;
}

int ring_buf_is_empty(ring_buf_t *rb) {
    unsigned long flags;
    spin_lock_irqsave(&rb->lock, &flags);
    int empty = (rb->tail == rb->head);
    spin_unlock_irqrestore(&rb->lock, flags);
    return empty;
}

int ring_buf_is_full(ring_buf_t *rb) {
    unsigned long flags;
    spin_lock_irqsave(&rb->lock, &flags);
    int full = ((rb->head + 1) % rb->capacity == rb->tail);
    spin_unlock_irqrestore(&rb->lock, flags);
    return full;
}

unsigned int ring_buf_count(ring_buf_t *rb) {
    unsigned long flags;
    spin_lock_irqsave(&rb->lock, &flags);
    unsigned int count = (rb->head - rb->tail + rb->capacity) % rb->capacity;
    spin_unlock_irqrestore(&rb->lock, flags);
    return count;
}
