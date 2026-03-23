#ifndef _BITMAP_H
#define _BITMAP_H

#include <stdint.h>

// Number of unsigned long words needed to hold n_bits bits.
// Use this to declare backing storage:
//   unsigned long storage[BITMAP_WORDS(N)];
#define BITMAP_WORDS(n_bits)  (((n_bits) + 63UL) / 64UL)

typedef struct {
    unsigned long *words;
    unsigned long  num_bits;
} bitmap_t;

// Initialise bm to use the provided storage array and zero all bits.
// storage must point to at least BITMAP_WORDS(num_bits) unsigned longs.
void bitmap_init(bitmap_t *bm, unsigned long *storage, unsigned long num_bits);

// Set bit n (mark as used).
void bitmap_set(bitmap_t *bm, unsigned long n);

// Clear bit n (mark as free).
void bitmap_clear(bitmap_t *bm, unsigned long n);

// Return 1 if bit n is set, 0 if clear.
int  bitmap_test(bitmap_t *bm, unsigned long n);

// Set `count` consecutive bits starting at `start`.
void bitmap_set_range(bitmap_t *bm, unsigned long start, unsigned long count);

// Clear `count` consecutive bits starting at `start`.
void bitmap_clear_range(bitmap_t *bm, unsigned long start, unsigned long count);

// Find the first run of `count` consecutive clear bits.
// Returns the starting index of the run, or -1 if no such run exists.
long bitmap_find_clear_run(bitmap_t *bm, unsigned long count);

#endif
