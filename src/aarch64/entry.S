/*
 * QEMU virt loads this image at 0x40080000. The boot stub jumps here
 * with x0 holding the device tree and the CPU already in EL1h.
 * If the mode is EL2 instead, eret drops into EL1h. Device addresses
 * are read from the tree.
 */
.section .text.init
.globl _start
_start:
	b 1f
	.word 0
	.quad 0x80000
	.quad 0x200000
	.quad 0
	.quad 0
	.quad 0
	.quad 0
	.word 0x644d5241
	.word 0
1:
	mrs x1, CurrentEL
	cmp x1, #8
	b.ne el1_entry

	mrs x1, hcr_el2
	orr x1, x1, #(1 << 31)
	bic x1, x1, #(1 << 4)
	bic x1, x1, #(1 << 3)
	msr hcr_el2, x1
	mrs x1, cnthctl_el2
	orr x1, x1, #3
	msr cnthctl_el2, x1
	msr cntvoff_el2, xzr
	mov x1, #0x3c5
	msr spsr_el2, x1
	adr x1, el1_entry
	msr elr_el2, x1
	eret

el1_entry:
	mrs x1, sctlr_el1
	bic x1, x1, #(1 << 1)
	bic x1, x1, #(1 << 2)
	bic x1, x1, #(1 << 3)
	bic x1, x1, #(1 << 4)
	bic x1, x1, #(1 << 12)
	bic x1, x1, #(1 << 19)
	msr sctlr_el1, x1
	isb
	ldr x1, =__bss_start
	ldr x2, =__bss_end
2:
	cmp x1, x2
	b.hs 3f
	str xzr, [x1], #8
	b 2b
3:
	ldr x1, =stack_top
	mov sp, x1
	ldr x1, =vector_table
	msr vbar_el1, x1
	isb
	bl main
	bl poweroff
