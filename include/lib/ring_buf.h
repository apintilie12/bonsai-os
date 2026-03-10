#ifndef _RING_BUF_H
#define _RING_BUF_H

#include "lib/spinlock.h"

typedef struct {
    void         *buf;
    unsigned int  capacity;   // number of elements
    unsigned int  elem_size;  // bytes per element
    unsigned int  head;       // write index
    unsigned int  tail;       // read index
    spinlock_t    lock;
} ring_buf_t;

#define RING_BUF_INIT(storage, cap, esize) \
    { .buf = (storage), .capacity = (cap), .elem_size = (esize), \
      .head = 0, .tail = 0, .lock = SPINLOCK_INIT }

int          ring_buf_push(ring_buf_t *rb, const void *elem);
int          ring_buf_pop(ring_buf_t *rb, void *out);
int          ring_buf_is_empty(ring_buf_t *rb);
int          ring_buf_is_full(ring_buf_t *rb);
unsigned int ring_buf_count(ring_buf_t *rb);

#endif
