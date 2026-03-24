#ifndef _KMALLOC_H
#define _KMALLOC_H

#include "mm.h"
#include "stdint.h"

#define KMALLOC_BLOCK_SIZE          32
#define KMALLOC_POOL_PAGES          64  // 64 * 4KB => 256KB Kernel heap
#define KMALLOC_POOL_SIZE           (KMALLOC_POOL_PAGES * PAGE_SIZE)
#define KMALLOC_NUM_BLOCKS          (KMALLOC_POOL_SIZE / KMALLOC_BLOCK_SIZE)
#define KMALLOC_BITMAP_SIZE         (KMALLOC_NUM_BLOCKS / 8) // 1024 bytes
#define KMALLOC_MAGIC               0xAB

typedef union _KMALLOC_HEADER {

    struct {
        uint16_t num_blocks;
        uint8_t magic;
    };
    uint8_t _padding[KMALLOC_BLOCK_SIZE];
}KMALLOC_HEADER;

void kmalloc_init(void);
void* kmalloc(uint64_t size);
void kfree(void* addr);

#endif //_KMALLOC_H