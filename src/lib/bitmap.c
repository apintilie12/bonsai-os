#include "lib/bitmap.h"

#define WORD_BITS  64UL
#define WORD(n)    ((n) >> 6)            // word index  = n / 64
#define BIT(n)     (1UL << ((n) & 63UL)) // bit mask within word

void bitmap_init(bitmap_t *bm, unsigned long *storage, unsigned long num_bits) {
    bm->words    = storage;
    bm->num_bits = num_bits;
    unsigned long nwords = BITMAP_WORDS(num_bits);
    for (unsigned long i = 0; i < nwords; i++) {
        storage[i] = 0;
    }
}

void bitmap_set(bitmap_t *bm, unsigned long n) {
    bm->words[WORD(n)] |= BIT(n);
}

void bitmap_clear(bitmap_t *bm, unsigned long n) {
    bm->words[WORD(n)] &= ~BIT(n);
}

int bitmap_test(bitmap_t *bm, unsigned long n) {
    return (bm->words[WORD(n)] & BIT(n)) != 0;
}

void bitmap_set_range(bitmap_t *bm, unsigned long start, unsigned long count) {
    for (unsigned long i = start; i < start + count; i++) {
        bitmap_set(bm, i);
    }
}

void bitmap_clear_range(bitmap_t *bm, unsigned long start, unsigned long count) {
    for (unsigned long i = start; i < start + count; i++) {
        bitmap_clear(bm, i);
    }
}

long bitmap_find_clear_run(bitmap_t *bm, unsigned long count) {
    unsigned long run_start = 0;
    unsigned long run_len   = 0;

    for (unsigned long i = 0; i < bm->num_bits; i++) {
        if (!bitmap_test(bm, i)) {
            if (run_len == 0) run_start = i;
            run_len++;
            if (run_len == count) return (long)run_start;
        } else {
            run_len = 0;
        }
    }
    return -1;
}
