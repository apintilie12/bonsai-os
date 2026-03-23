#ifndef	_MM_H
#define	_MM_H

#include "peripherals/base.h"
#include "lib/types.h"

#define VA_START 			0xffff000000000000

#define PHYS_MEMORY_SIZE 		0x40000000	

#define PAGE_MASK			0xfffffffffffff000
#define PAGE_SHIFT	 		12
#define TABLE_SHIFT 			9
#define SECTION_SHIFT			(PAGE_SHIFT + TABLE_SHIFT)

#define PAGE_SIZE   			(1 << PAGE_SHIFT)	
#define SECTION_SIZE			(1 << SECTION_SHIFT)	

#define LOW_MEMORY              	(2 * SECTION_SIZE)
#define HIGH_MEMORY             	DEVICE_BASE

#define PAGING_MEMORY 			(HIGH_MEMORY - LOW_MEMORY)
#define PAGING_PAGES 			(PAGING_MEMORY/PAGE_SIZE)

#define PTRS_PER_TABLE			(1 << TABLE_SHIFT)

#define PGD_SHIFT			PAGE_SHIFT + 3*TABLE_SHIFT
#define PUD_SHIFT			PAGE_SHIFT + 2*TABLE_SHIFT
#define PMD_SHIFT			PAGE_SHIFT + TABLE_SHIFT

#define PG_DIR_SIZE			(4 * PAGE_SIZE)

#define FRAME_IDX(phys_addr) ((phys_addr - LOW_MEMORY) / PAGE_SIZE)

#define FRAME_FREE          0x00
#define FRAME_KERNEL        0x01
#define FRAME_USER          0x02
#define FRAME_PAGETABLE     0x03

#ifndef __ASSEMBLER__

typedef struct _FRAME_DESC {
    uint8_t flags;
    uint8_t refcount;
    int16_t owner_pid;
} FRAME_DESC;

typedef struct _MEM_STATS {
    int64_t total;
    int64_t used;
    int64_t free;
    int64_t kernel;
    int64_t user;
    int64_t pagetable;
} MEM_STATS;


#include "kernel/sched.h"

unsigned long get_free_page();
void free_page(unsigned long p);
void map_page(TASK_STRUCT *task, unsigned long va, unsigned long page);
void memzero(unsigned long src, unsigned long n);
void memcpy(unsigned long dst, unsigned long src, unsigned long n);
void get_mem_stats(MEM_STATS* stats);

int copy_virt_memory(TASK_STRUCT *dst); 
unsigned long allocate_kernel_page(); 
unsigned long allocate_user_page(TASK_STRUCT *task, unsigned long va); 

extern unsigned long pg_dir;

#endif

#endif  /*_MM_H */
