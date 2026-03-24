#include "mm/kmalloc.h"

#include <stddef.h>

#include "lib/bitmap.h"
#include "lib/panic.h"
#include "lib/spinlock.h"

static bitmap_t          bitmap;
static unsigned long     bitmap_storage[BITMAP_WORDS(KMALLOC_NUM_BLOCKS)];
static spinlock_t        bitmap_lock = SPINLOCK_INIT;
static unsigned long     pool_base;

void kmalloc_init(void) {
    bitmap_init(&bitmap, bitmap_storage, KMALLOC_NUM_BLOCKS);
    pool_base = get_free_pages(KMALLOC_POOL_PAGES);
    ASSERT(pool_base != 0); // Check pages could be allocated
    pool_base += VA_START; // Make the address a VA
}
void* kmalloc(uint64_t size) {
    if (size == 0) {
        return NULL;
    }
    if (size > PAGE_SIZE) {
        return (void*)(get_free_page() + VA_START);
    }
    int total = size + KMALLOC_BLOCK_SIZE;
    int num_blocks = (total + KMALLOC_BLOCK_SIZE - 1) / KMALLOC_BLOCK_SIZE;
    spin_lock(&bitmap_lock);
    long first_block = bitmap_find_clear_run(&bitmap, num_blocks);
    if (first_block == -1) {
        spin_unlock(&bitmap_lock);
        return NULL;
    }
    bitmap_set_range(&bitmap, first_block, num_blocks);
    spin_unlock(&bitmap_lock);

    KMALLOC_HEADER *hdr = (KMALLOC_HEADER *)(pool_base + first_block * KMALLOC_BLOCK_SIZE);
    hdr->num_blocks = (uint16_t)num_blocks;
    hdr->magic      = KMALLOC_MAGIC;

    return (void *)((unsigned long)hdr + KMALLOC_BLOCK_SIZE);
}

void kfree(void *addr) {
    if (addr == NULL) return;

    unsigned long a = (unsigned long)addr;

    if (a < pool_base || a >= pool_base + KMALLOC_POOL_SIZE) {
        free_page(a - VA_START);
        return;
    }

    KMALLOC_HEADER *hdr = (KMALLOC_HEADER *)(a - KMALLOC_BLOCK_SIZE);
    ASSERT(hdr->magic == KMALLOC_MAGIC);

    uint16_t num_blocks = hdr->num_blocks;
    int first_block = ((unsigned long)hdr - pool_base) / KMALLOC_BLOCK_SIZE;

    hdr->magic = 0;

    spin_lock(&bitmap_lock);
    bitmap_clear_range(&bitmap, first_block, num_blocks);
    spin_unlock(&bitmap_lock);
}
