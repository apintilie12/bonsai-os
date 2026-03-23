#include "mm/mm.h"

#include <lib/panic.h>

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
		task->mm.kernel_pages[task->mm.kernel_pages_count++] = task->mm.pgd;
	}
	pgd = task->mm.pgd;
	int new_table;
	unsigned long pud = map_table((unsigned long *)(pgd + VA_START), PGD_SHIFT, va, &new_table);
	if (new_table) {
		task->mm.kernel_pages[task->mm.kernel_pages_count++] = pud;
	}
	unsigned long pmd = map_table((unsigned long *)(pud + VA_START) , PUD_SHIFT, va, &new_table);
	if (new_table) {
		task->mm.kernel_pages[task->mm.kernel_pages_count++] = pmd;
	}
	unsigned long pte = map_table((unsigned long *)(pmd + VA_START), PMD_SHIFT, va, &new_table);
	if (new_table) {
		task->mm.kernel_pages[task->mm.kernel_pages_count++] = pte;
	}
	map_table_entry((unsigned long *)(pte + VA_START), va, page);
}

// Walk the 4-level page table for a task and return the physical address
// mapped at va, or 0 if not mapped.
static unsigned long pt_walk(TASK_STRUCT *task, unsigned long va) {
	if (!task->mm.pgd) return 0;

	unsigned long *pgd = (unsigned long *)(task->mm.pgd + VA_START);
	unsigned long pgd_idx = (va >> PGD_SHIFT) & (PTRS_PER_TABLE - 1);
	if (!pgd[pgd_idx]) return 0;

	unsigned long *pud = (unsigned long *)((pgd[pgd_idx] & PAGE_MASK) + VA_START);
	unsigned long pud_idx = (va >> PUD_SHIFT) & (PTRS_PER_TABLE - 1);
	if (!pud[pud_idx]) return 0;

	unsigned long *pmd = (unsigned long *)((pud[pud_idx] & PAGE_MASK) + VA_START);
	unsigned long pmd_idx = (va >> PMD_SHIFT) & (PTRS_PER_TABLE - 1);
	if (!pmd[pmd_idx]) return 0;

	unsigned long *pte = (unsigned long *)((pmd[pmd_idx] & PAGE_MASK) + VA_START);
	unsigned long pte_idx = (va >> PAGE_SHIFT) & (PTRS_PER_TABLE - 1);
	if (!pte[pte_idx]) return 0;

	return pte[pte_idx] & PAGE_MASK;
}

int copy_virt_memory(TASK_STRUCT *dst) {
	TASK_STRUCT *src = get_current_task();

	dst->mm.vma_count = src->mm.vma_count;
	for (int i = 0; i < src->mm.vma_count; i++) {
		dst->mm.vmas[i] = src->mm.vmas[i];
	}

	for (int i = 0; i < src->mm.vma_count; i++) {
		for (unsigned long va = src->mm.vmas[i].virt_start;
		     va < src->mm.vmas[i].virt_end;
		     va += PAGE_SIZE) {

			if (pt_walk(src, va) == 0) continue;

			unsigned long dst_kva = allocate_user_page(dst, va);
			if (dst_kva == 0) return E_NOMEM;

			memcpy(dst_kva, va, PAGE_SIZE);
		}
	}
	return E_OK;
}

int do_mem_abort(unsigned long addr, unsigned long esr) {
	TASK_STRUCT *current = get_current_task();
	unsigned long dfs = (esr & 0x3F);
	if ((dfs & 0x3C) != 0x04) {
		return E_INVAL;
	}

	unsigned long fault_addr = addr & PAGE_MASK;
	if (vma_find(&current->mm, fault_addr) == NULL) {
		return E_INVAL;
	}

	unsigned long page = get_free_page();
	if (page == 0) {
		return E_NOMEM;
	}

	spin_lock(&mem_map_lock);
	mem_map[FRAME_IDX(page)].flags = FRAME_USER;
	spin_unlock(&mem_map_lock);

	map_page(current, fault_addr, page);
	return E_OK;
}

int vma_add(MM_STRUCT* mm, uint64_t va_start, uint64_t va_end, uint32_t flags) {
	ASSERT(va_start < va_end);
	ASSERT((va_start & (PAGE_SIZE - 1)) == 0);
	ASSERT((va_end & (PAGE_SIZE - 1)) == 0);

	if (mm->vma_count >= MAX_VMA_COUNT) {
		return E_NOMEM;
	}

	for (int i = 0; i < mm->vma_count; i++) {
		if (va_start < mm->vmas[i].virt_end && va_end > mm->vmas[i].virt_start) {
			return E_INVAL;
		}
	}

	mm->vmas[mm->vma_count] =(VM_AREA){va_start, va_end, flags};
	mm->vma_count++;
	return E_OK;
}

VM_AREA* vma_find(MM_STRUCT *mm, uint64_t addr) {
	for (int i = 0; i < mm->vma_count; i++) {
		if (mm->vmas[i].virt_start <= addr && addr < mm->vmas[i].virt_end) {
			return &mm->vmas[i];
		}
	}
	return NULL;
}

void vma_remove(MM_STRUCT *mm, uint64_t va_start) {
	int idx = -1;
	for (int i = 0; i < mm->vma_count; i++) {
		if (mm->vmas[i].virt_start == va_start) {
			idx = i;
			break;
		}
	}
	if (idx == -1) return;

	for (int i = idx; i < mm->vma_count - 1; i++) {
		mm->vmas[i] = mm->vmas[i + 1];
	}
	mm->vma_count--;
	mm->vmas[mm->vma_count] = (VM_AREA){0, 0, 0};
}