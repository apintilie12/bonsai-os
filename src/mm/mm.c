#include "mm/mm.h"
#include "arch/arm/mmu.h"
#include "kernel/sched.h"
#include "lib/spinlock.h"
#include "lib/errno.h"

static FRAME_DESC mem_map[PAGING_PAGES] = {0,};
static spinlock_t mem_map_lock = SPINLOCK_INIT;

void get_mem_stats(MEM_STATS* stats) {
	stats->total = PAGING_PAGES;
	int64_t count_kernel = 0;
	int64_t count_user = 0;
	int64_t count_pagetable = 0;
	spin_lock(&mem_map_lock);
	for (unsigned long i = 0; i < PAGING_PAGES; i++) {
		switch (mem_map[i].flags) {
			case FRAME_KERNEL:
				count_kernel++;
				break;
			case FRAME_USER:
				count_user++;
				break;
			case FRAME_PAGETABLE:
				count_pagetable++;
				break;
			default:
				break;
		}
	}
	spin_unlock(&mem_map_lock);
	stats->kernel = count_kernel;
	stats->user = count_user;
	stats->pagetable = count_pagetable;
	stats->used = count_kernel + count_user + count_pagetable;
	stats->free = stats->total - stats->used;
}

unsigned long allocate_kernel_page() {
	unsigned long page = get_free_page();
	if (page == 0) {
		return 0;
	}
	return page + VA_START;
}

unsigned long allocate_user_page(TASK_STRUCT *task, unsigned long va) {
	unsigned long page = get_free_page();
	if (page == 0) {
		return 0;
	}

	spin_lock(&mem_map_lock);
	mem_map[FRAME_IDX(page)].flags = FRAME_USER;
	spin_unlock(&mem_map_lock);

	map_page(task, va, page);
	return page + VA_START;
}

unsigned long get_free_page()
{
	spin_lock(&mem_map_lock);
	for (int i = 0; i < PAGING_PAGES; i++){
		if (mem_map[i].flags == FRAME_FREE){
			mem_map[i].flags = FRAME_KERNEL;
			mem_map[i].refcount = 1;
			mem_map[i].owner_pid = -1;
			spin_unlock(&mem_map_lock);
			unsigned long page = LOW_MEMORY + i*PAGE_SIZE;
			memzero(page + VA_START, PAGE_SIZE);
			return page;
		}
	}
	spin_unlock(&mem_map_lock);
	return 0;
}

void free_page(unsigned long p){
	spin_lock(&mem_map_lock);
	int idx = FRAME_IDX(p);
	mem_map[idx].flags = FRAME_FREE;
	mem_map[idx].refcount = 0;
	mem_map[idx].owner_pid = -1;
	spin_unlock(&mem_map_lock);
}

void map_table_entry(unsigned long *pte, unsigned long va, unsigned long pa) {
	unsigned long index = va >> PAGE_SHIFT;
	index = index & (PTRS_PER_TABLE - 1);
	unsigned long entry = pa | MMU_PTE_FLAGS; 
	pte[index] = entry;
}

unsigned long map_table(unsigned long *table, unsigned long shift, unsigned long va, int* new_table) {
	unsigned long index = va >> shift;
	index = index & (PTRS_PER_TABLE - 1);
	if (!table[index]){
		*new_table = 1;
		unsigned long next_level_table = get_free_page();
		spin_lock(&mem_map_lock);
		mem_map[FRAME_IDX(next_level_table)].flags = FRAME_PAGETABLE;
		spin_unlock(&mem_map_lock);
		unsigned long entry = next_level_table | MM_TYPE_PAGE_TABLE;
		table[index] = entry;
		return next_level_table;
	} else {
		*new_table = 0;
	}
	return table[index] & PAGE_MASK;
}

void map_page(TASK_STRUCT *task, unsigned long va, unsigned long page){
	unsigned long pgd;
	if (!task->mm.pgd) {
		task->mm.pgd = get_free_page();
		task->mm.kernel_pages[++task->mm.kernel_pages_count] = task->mm.pgd;
	}
	pgd = task->mm.pgd;
	int new_table;
	unsigned long pud = map_table((unsigned long *)(pgd + VA_START), PGD_SHIFT, va, &new_table);
	if (new_table) {
		task->mm.kernel_pages[++task->mm.kernel_pages_count] = pud;
	}
	unsigned long pmd = map_table((unsigned long *)(pud + VA_START) , PUD_SHIFT, va, &new_table);
	if (new_table) {
		task->mm.kernel_pages[++task->mm.kernel_pages_count] = pmd;
	}
	unsigned long pte = map_table((unsigned long *)(pmd + VA_START), PMD_SHIFT, va, &new_table);
	if (new_table) {
		task->mm.kernel_pages[++task->mm.kernel_pages_count] = pte;
	}
	map_table_entry((unsigned long *)(pte + VA_START), va, page);
	USER_PAGE p = {page, va};
	task->mm.user_pages[task->mm.user_pages_count++] = p;
}

int copy_virt_memory(TASK_STRUCT *dst) {
	TASK_STRUCT* src = get_current_task();
	for (int i = 0; i < src->mm.user_pages_count; i++) {
		unsigned long kernel_va = allocate_user_page(dst, src->mm.user_pages[i].virt_addr);
		if( kernel_va == 0) {
			return E_NOMEM;
		}
		memcpy(kernel_va, src->mm.user_pages[i].virt_addr, PAGE_SIZE);
	}
	return 0;
}

static int ind = 1;

int do_mem_abort(unsigned long addr, unsigned long esr) {
	TASK_STRUCT *current = get_current_task();
	unsigned long dfs = (esr & 0b111111);
	if ((dfs & 0b111100) == 0b100) {
		unsigned long page = get_free_page();
		if (page == 0) {
			return E_NOMEM;
		}
		map_page(current, addr & PAGE_MASK, page);
		ind++;
		if (ind > 2){
			return E_INVAL;
		}
		return E_OK;
	}
	return E_INVAL;
}
