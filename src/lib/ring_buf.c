#include "lib/ring_buf.h"

static void rb_memcpy(void *dst, const void *src, unsigned int n) {
    unsigned char *d = (unsigned char *)dst;
    const unsigned char *s = (const unsigned char *)src;
    while (n--) *d++ = *s++;
}

int ring_buf_push(ring_buf_t *rb, const void *elem) {
    spin_lock(&rb->lock);
    unsigned int next = (rb->head + 1) % rb->capacity;
    if (next == rb->tail) {
        spin_unlock(&rb->lock);
        return 0;  // full, drop
    }
    unsigned char *slot = (unsigned char *)rb->buf + rb->head * rb->elem_size;
    rb_memcpy(slot, elem, rb->elem_size);
    rb->head = next;
    spin_unlock(&rb->lock);
    return 1;
}

int ring_buf_pop(ring_buf_t *rb, void *out) {
    spin_lock(&rb->lock);
    if (rb->tail == rb->head) {
        spin_unlock(&rb->lock);
        return 0;  // empty
    }
    unsigned char *slot = (unsigned char *)rb->buf + rb->tail * rb->elem_size;
    rb_memcpy(out, slot, rb->elem_size);
    rb->tail = (rb->tail + 1) % rb->capacity;
    spin_unlock(&rb->lock);
    return 1;
}

// offset 0 = most recently pushed, 1 = one before that, etc.
int ring_buf_peek(ring_buf_t *rb, unsigned int offset, void *out) {
    spin_lock(&rb->lock);
    unsigned int count = (rb->head - rb->tail + rb->capacity) % rb->capacity;
    if (offset >= count) {
        spin_unlock(&rb->lock);
        return 0;
    }
    unsigned int idx = (rb->head - 1 - offset + rb->capacity) % rb->capacity;
    unsigned char *slot = (unsigned char *)rb->buf + idx * rb->elem_size;
    rb_memcpy(out, slot, rb->elem_size);
    spin_unlock(&rb->lock);
    return 1;
}

int ring_buf_is_empty(ring_buf_t *rb) {
    spin_lock(&rb->lock);
    int empty = (rb->tail == rb->head);
    spin_unlock(&rb->lock);
    return empty;
}

int ring_buf_is_full(ring_buf_t *rb) {
    spin_lock(&rb->lock);
    int full = ((rb->head + 1) % rb->capacity == rb->tail);
    spin_unlock(&rb->lock);
    return full;
}

unsigned int ring_buf_count(ring_buf_t *rb) {
    spin_lock(&rb->lock);
    unsigned int count = (rb->head - rb->tail + rb->capacity) % rb->capacity;
    spin_unlock(&rb->lock);
    return count;
}
