#include "arm/mmu.h"
#include "arm/sysregs.h"
#include "mm.h"
#include "peripherals/base.h"

.data
.balign 8
.globl secondary_boot_flag
secondary_boot_flag: .quad 0

.section ".text.boot"

.globl _start
_start:
	mrs     x0, CurrentEL
	lsr     x0, x0, #2 		// The EL is in bits [3:2]

	cmp		x0, #3 			// Check if we are in EL3 (running on physical HW)
	b.eq 	el3_entry

	cmp 	x0, #2			// Check if we are in EL2 (running in QEMU)
	b.eq 	el2_entry

	// If neither, something is wrong -> hang

proc_hang: 
	b proc_hang

secondary_spin:
	// Secondary cores spin here until master sets the flag
	ldr	x1, =secondary_boot_flag
	ldr	x2, =VA_START
	sub	x1, x1, x2		// Convert virtual address to physical
1:	wfe
	ldr	x0, [x1]
	cbz	x0, 1b
	b	secondary_entry


el3_entry:
	ldr	x0, =SCTLR_VALUE_MMU_DISABLED
	msr	sctlr_el1, x0

	ldr	x0, =HCR_VALUE
	msr	hcr_el2, x0

	ldr	x0, =SCR_VALUE
	msr	scr_el3, x0

	ldr	x0, =SPSR_VALUE_EL1
	msr	spsr_el3, x0

	adr	x0, el1_entry
	msr	elr_el3, x0

	// Enable SMP/cache coherency (CPUECTLR_EL1.SMPEN) while still in Secure EL3
	mrs	x0, s3_1_c15_c2_1
	orr	x0, x0, #(1 << 6)
	msr	s3_1_c15_c2_1, x0
	isb

	eret

el2_entry:
	ldr	x0, =SCTLR_VALUE_MMU_DISABLED
	msr	sctlr_el1, x0

	ldr     x0, =HCR_VALUE
    msr     hcr_el2, x0

	ldr	x0, =SPSR_VALUE_EL1
	msr	spsr_el2, x0

	adr x0, el1_entry
	msr elr_el2, x0

	// Enable SMP/cache coherency (CPUECTLR_EL1.SMPEN) while still at EL2
	mrs	x0, s3_1_c15_c2_1
	orr	x0, x0, #(1 << 6)
	msr	s3_1_c15_c2_1, x0
	isb

	eret


el1_entry:
    mrs	x0, mpidr_el1
	and	x0, x0,#0xFF		// Check processor id
	cbz	x0, master_init		// Hang for all non-primary CPU
	b	secondary_spin

master_init:
    // DEBUG: Setup temp stack and init UART
    mov sp, #0x80000
    bl init_uart_phys
    mov w0, #'A'
    bl putc_phys
    // If you see 'A', UART init is working

	adr	x0, bss_begin
	adr	x1, bss_end
	sub	x1, x1, x0
	bl 	memzero

	bl 	__create_page_tables
	dsb ish

    mov w0, #'B'
    bl putc_phys
    // If you see 'B', page tables were created successfully

    // Signal secondary cores to wake up
	ldr	x0, =secondary_boot_flag
	ldr x2, =VA_START
	sub x0, x0, x2 //Convert virtual address to physical
	mov	x1, #1
	str	x1, [x0]
	sev				// Send Event to wake up secondaries

secondary_entry:
	// All cores (Master + Secondaries) run this to enable MMU
    mov w0, #'C'
    bl putc_phys // C means sev was sent

	mov	x0, #VA_START			
	add	sp, x0, #LOW_MEMORY

	// Calculate unique stack pointer for each core: sp = LOW_MEMORY - (cpu_id * PAGE_SIZE)
    mrs	x1, mpidr_el1
    and	x1, x1, #0xFF
    mov	x2, #PAGE_SIZE
    mul	x2, x1, x2
    sub	sp, sp, x2

    mov w0, #'D'
    bl putc_phys // D means stack pointers were computed

	adrp	x0, pg_dir				
	msr	ttbr1_el1, x0

    ldr	x0, =(TCR_VALUE)
	msr	tcr_el1, x0

	ldr	x0, =(MAIR_VALUE)
	msr	mair_el1, x0


    mov w0, #'E'
    bl putc_phys
    // If you see 'E', TTBR/TCR/MAIR setup is done

    mrs	x0, mpidr_el1
	and	x0, x0, #0xFF
	cbz x0, 1f
    ldr x2, =secondary_start
    b 2f

1:  ldr	x2, =kernel_main

2:	ldr	x0, =SCTLR_VALUE_MMU_ENABLED
	msr	sctlr_el1, x0
	//isb

	br 	x2

	.macro	create_pgd_entry, tbl, virt, tmp1, tmp2
	create_table_entry \tbl, \virt, PGD_SHIFT, \tmp1, \tmp2
	create_table_entry \tbl, \virt, PUD_SHIFT, \tmp1, \tmp2
	.endm

	.macro	create_table_entry, tbl, virt, shift, tmp1, tmp2
	lsr	\tmp1, \virt, #\shift
	and	\tmp1, \tmp1, #PTRS_PER_TABLE - 1			// table index
	add	\tmp2, \tbl, #PAGE_SIZE
	orr	\tmp2, \tmp2, #MM_TYPE_PAGE_TABLE
	str	\tmp2, [\tbl, \tmp1, lsl #3]
	add	\tbl, \tbl, #PAGE_SIZE					// next level table page
	.endm

	.macro	create_block_map, tbl, phys, start, end, flags, tmp1
	lsr	\start, \start, #SECTION_SHIFT
	and	\start, \start, #PTRS_PER_TABLE - 1			// table index
	lsr	\end, \end, #SECTION_SHIFT
	and	\end, \end, #PTRS_PER_TABLE - 1				// table end index
	lsr	\phys, \phys, #SECTION_SHIFT
	mov	\tmp1, #\flags
	orr	\phys, \tmp1, \phys, lsl #SECTION_SHIFT			// table entry
9999:	str	\phys, [\tbl, \start, lsl #3]				// store the entry
	add	\start, \start, #1					// next entry
	add	\phys, \phys, #SECTION_SIZE				// next block
	cmp	\start, \end
	b.ls	9999b
	.endm

__create_page_tables:
	mov	x29, x30						// save return address

	adrp	x0, pg_dir
	mov	x1, #PG_DIR_SIZE
	bl 	memzero

	adrp	x0, pg_dir
	mov	x1, #VA_START
	create_pgd_entry x0, x1, x2, x3

	/* Mapping kernel and init stack*/
	mov 	x1, xzr							// start mapping from physical offset 0
	mov 	x2, #VA_START						// first virtual address
	ldr	x3, =(VA_START + DEVICE_BASE - SECTION_SIZE)		// last virtual address
	create_block_map x0, x1, x2, x3, MMU_FLAGS, x4

	/* Mapping device memory*/
	mov 	x1, #DEVICE_BASE					// start mapping from device base address
	ldr 	x2, =(VA_START + DEVICE_BASE)				// first virtual address
	ldr	x3, =(VA_START + PHYS_MEMORY_SIZE - SECTION_SIZE)	// last virtual address
	create_block_map x0, x1, x2, x3, MMU_DEVICE_FLAGS, x4

	/* Mapping 2nd GB (for ARM Local Peripherals at 0x40000000) */

	/* Link PUD[1] to the new PMD table (4th page) */
	adrp	x0, pg_dir
	add	x0, x0, #PAGE_SIZE			// x0 points to PUD table
	adrp	x1, pg_dir
	add	x1, x1, #(3 * PAGE_SIZE)		// x1 points to new PMD table (Page 4)
	orr	x1, x1, #MM_TYPE_PAGE_TABLE
	str	x1, [x0, #8]				// Store at index 1 (8 bytes offset)

	/* Map the QA7 block in the new PMD table */
	adrp	x0, pg_dir
	add	x0, x0, #(3 * PAGE_SIZE)		// x0 points to new PMD table
	mov	x1, #0x40000000				// Physical Address
	ldr	x2, =(VA_START + 0x40000000)		// Virtual Address
	ldr	x3, =(VA_START + 0x40000000)		// End (Map just 1 block)
	create_block_map x0, x1, x2, x3, MMU_DEVICE_FLAGS, x4

	mov	x30, x29						// restore return address
	ret

// ============================================================================
// EARLY DEBUG UART (Physical Addresses, MMU OFF)
// ============================================================================

.equ PBASE_PHYS, 0x3F000000
.equ GPFSEL1_PHYS, (PBASE_PHYS + 0x00200004)
.equ GPPUD_PHYS, (PBASE_PHYS + 0x00200094)
.equ GPPUDCLK0_PHYS, (PBASE_PHYS + 0x00200098)
.equ AUX_ENABLES_PHYS, (PBASE_PHYS + 0x00215004)
.equ AUX_MU_IO_REG_PHYS, (PBASE_PHYS + 0x00215040)
.equ AUX_MU_IER_REG_PHYS, (PBASE_PHYS + 0x00215044)
.equ AUX_MU_IIR_REG_PHYS, (PBASE_PHYS + 0x00215048)
.equ AUX_MU_LCR_REG_PHYS, (PBASE_PHYS + 0x0021504C)
.equ AUX_MU_MCR_REG_PHYS, (PBASE_PHYS + 0x00215050)
.equ AUX_MU_LSR_REG_PHYS, (PBASE_PHYS + 0x00215054)
.equ AUX_MU_CNTL_REG_PHYS, (PBASE_PHYS + 0x00215060)
.equ AUX_MU_BAUD_REG_PHYS, (PBASE_PHYS + 0x00215068)

.globl init_uart_phys
init_uart_phys:
    mov x10, x30 // Save LR manually

    // GPIO Setup
    ldr x0, =GPFSEL1_PHYS
    ldr w1, [x0]
    and w1, w1, #~(7 << 12) // Clean gpio14
    orr w1, w1, #(2 << 12)  // Alt5
    and w1, w1, #~(7 << 15) // Clean gpio15
    orr w1, w1, #(2 << 15)  // Alt5
    str w1, [x0]

    ldr x0, =GPPUD_PHYS
    str wzr, [x0]
    mov x0, #150
    bl delay_phys

    ldr x0, =GPPUDCLK0_PHYS
    mov w1, #(1 << 14)
    orr w1, w1, #(1 << 15)
    str w1, [x0]
    mov x0, #150
    bl delay_phys

    ldr x0, =GPPUDCLK0_PHYS
    str wzr, [x0]

    // AUX Enable & Config
    ldr x0, =AUX_ENABLES_PHYS
    mov w1, #1
    str w1, [x0]
    ldr x0, =AUX_MU_CNTL_REG_PHYS
    str wzr, [x0]
    ldr x0, =AUX_MU_IER_REG_PHYS
    str wzr, [x0]
    ldr x0, =AUX_MU_LCR_REG_PHYS
    mov w1, #3
    str w1, [x0]
    ldr x0, =AUX_MU_MCR_REG_PHYS
    str wzr, [x0]
    ldr x0, =AUX_MU_BAUD_REG_PHYS
    mov w1, #270
    str w1, [x0]
    ldr x0, =AUX_MU_CNTL_REG_PHYS
    mov w1, #3
    str w1, [x0]

    mov x30, x10 // Restore LR
    ret

delay_phys:
    subs x0, x0, #1
    bne delay_phys
    ret

.globl putc_phys
putc_phys:
    // Expects char in w0
    ldr x1, =AUX_MU_LSR_REG_PHYS
1:  ldr w2, [x1]
    and w2, w2, #0x20
    cbz w2, 1b
    ldr x1, =AUX_MU_IO_REG_PHYS
    str w0, [x1]
    ret
