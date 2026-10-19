/*
 * QEMU virt loads this image at 0x40010000 and jumps here from its
 * boot stub. r2 holds the device tree. The stub leaves the CPU in SVC.
 * If the mode is Hyp instead, eret drops into SVC. Device addresses
 * are read from the tree.
 */
.syntax unified
.arm

.section .text.init
.globl _start
_start:
	b 1f
	.word 0, 0, 0, 0, 0, 0, 0, 0
	.word 0x016f2818
	.word _start
	.word _end
1:
	cpsid if
	mrs r0, cpsr
	and r1, r0, #0x1f
	cmp r1, #0x1a
	bne svc_entry

	mrc p15, 4, r3, c1, c1, 0
	bic r3, r3, #(1 << 4)
	bic r3, r3, #(1 << 3)
	mcr p15, 4, r3, c1, c1, 0
	mrc p15, 4, r3, c14, c1, 0
	orr r3, r3, #3
	mcr p15, 4, r3, c14, c1, 0
	mov r3, #0
	mcrr p15, 4, r3, r3, c14

	mov r0, #0xd3
	msr spsr_cxsf, r0
	ldr r0, =svc_entry
	msr elr_hyp, r0
	.arch_extension virt
	eret

svc_entry:
	ldr sp, =stack_svc_top
	msr cpsr_c, #0xd2
	ldr sp, =stack_irq_top
	msr cpsr_c, #0xd7
	ldr sp, =stack_abt_top
	msr cpsr_c, #0xdb
	ldr sp, =stack_und_top
	msr cpsr_c, #0xd3

	mrc p15, 0, r0, c1, c0, 0
	bic r0, r0, #(1 << 0)
	bic r0, r0, #(1 << 1)
	bic r0, r0, #(1 << 2)
	bic r0, r0, #(1 << 12)
	mcr p15, 0, r0, c1, c0, 0
	isb

	ldr r0, =vector_table
	mcr p15, 0, r0, c12, c0, 0

	ldr r0, =__bss_start
	ldr r1, =__bss_end
	mov r3, #0
1:
	cmp r0, r1
	bhs 2f
	str r3, [r0], #4
	b 1b
2:
	mov r0, r2
	bl main
	bl poweroff
