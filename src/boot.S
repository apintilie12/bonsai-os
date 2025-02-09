#include "mm.h"

.section ".text.boot"

.globl _start
_start:
	mrs	x0, mpidr_el1		
	and	x0, x0,#0xFF		// Check processor id
	cbz	x0, master		// Hang for all non-primary CPU
	b	secondary

proc_hang: 
	b 	proc_hang

secondary:
	mrs x0, mpidr_el1
	and x0, x0, #0xFF	//Get processor ID
	mov sp, #LOW_MEMORY
	mov x1, x0   		//set sp = LOW_MEMORY - cpuid * STACK_SIZE
	start_loop:
		sub sp, sp, #STACK_SIZE
		sub x1, x1, #1
		cmp x1, #0
		bgt start_loop
	bl kernel_main
	b proc_hang

master:
	adr	x0, bss_begin
	adr	x1, bss_end
	sub	x1, x1, x0
	bl 	memzero

	mov	sp, #LOW_MEMORY 
	mov x0, #0
	bl	kernel_main
	b 	proc_hang		// should never come here
