/*
 * QEMU virt loads this image at 0x40080000. The boot stub jumps here
 * with x0 holding the device tree and the CPU already in EL1h.
 * If the mode is EL2 instead, eret drops into EL1h. Device addresses
 * are read from the tree.
 */
.section .text.init
.globl _start
_start:
	/*
	 * Linux Image header. text_offset is 0x80000, so the stub at
	 * 0x40000000 jumps to 0x40080000. image_size reserves 2MiB, which
	 * covers BSS. Magic 0x644d5241 ("ARM\x64") is at file offset 56.
	 * x0 is the device tree and must still be x0 when main is called.
	 */
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
	/*
	 * CurrentEL bits [3:2] are the exception level. The value 8 is
	 * EL2. This board's stub arrives in EL1h (CurrentEL 4) with DAIF
	 * set, so the drop below is skipped.
	 */
	mrs x1, CurrentEL
	cmp x1, #8
	b.ne el1_entry

	/*
	 * EL2 fallback. HCR_EL2.RW selects AArch64 at EL1. Clearing IMO
	 * and FMO leaves IRQs for EL1. CNTHCTL enables the physical timer
	 * for the lower level, and CNTVOFF is zero so the counter matches.
	 * SPSR 0x3c5 is EL1h with DAIF set. eret lands at el1_entry.
	 */
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
	/*
	 * With the MMU off, memory is Device-nGnRnE and an unaligned
	 * store faults. Clear SCTLR_EL1.A, C, SA, SA0, I, and WXN before
	 * C runs. Caches stay off. x0 is left alone.
	 */
	mrs x1, sctlr_el1
	bic x1, x1, #(1 << 1)
	bic x1, x1, #(1 << 2)
	bic x1, x1, #(1 << 3)
	bic x1, x1, #(1 << 4)
	bic x1, x1, #(1 << 12)
	bic x1, x1, #(1 << 19)
	msr sctlr_el1, x1
	isb
	/*
	 * BSS is cleared eight bytes at a time. The kernel stack lives
	 * in BSS, so this loop finishes before sp is loaded.
	 */
	ldr x1, =__bss_start
	ldr x2, =__bss_end
2:
	cmp x1, x2
	b.hs 3f
	str xzr, [x1], #8
	b 2b
3:
	/*
	 * One EL1 stack. VBAR_EL1 is 2KiB-aligned and points at the
	 * sixteen vector slots. main still sees the device tree in x0.
	 * A return from main is the shell's exit.
	 */
	ldr x1, =stack_top
	mov sp, x1
	ldr x1, =vector_table
	msr vbar_el1, x1
	isb
	bl main
	bl poweroff
