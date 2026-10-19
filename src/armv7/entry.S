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
	/*
	 * Linux ARM image header. The branch skips the header. Magic
	 * 0x016f2818 sits at file offset 0x24, which is what the virt
	 * stub looks for. The stub then jumps to the load address,
	 * 0x40010000, with r2 pointing at the device tree.
	 */
	b 1f
	.word 0, 0, 0, 0, 0, 0, 0, 0
	.word 0x016f2818
	.word _start
	.word _end
1:
	/*
	 * The low 5 bits of CPSR are the mode. 0x1a is Hyp, 0x13 is SVC.
	 * This board's stub arrives in SVC with I and F set, so the Hyp
	 * drop is skipped and execution continues at svc_entry. r2 still
	 * holds the device tree.
	 */
	cpsid if
	mrs r0, cpsr
	and r1, r0, #0x1f
	cmp r1, #0x1a
	bne svc_entry

	/*
	 * Hyp fallback. HCR.IMO and HCR.FMO would keep IRQs in Hyp;
	 * clearing them lets SVC take the timer. CNTHCTL lets the lower
	 * level read the physical counter. SPSR 0xd3 is SVC with I and F
	 * set. eret lands at svc_entry.
	 */
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
	/*
	 * Each mode has its own stack pointer. SVC gets the kernel stack.
	 * IRQ, abort, and undefined get smaller ones, so a fault taken
	 * while SVC is busy still has registers to save into. cpsr_c
	 * writes the mode bits; 0xd2 is IRQ, 0xd7 is abort, 0xdb is
	 * undefined, and 0xd3 returns to SVC with I and F set.
	 */
	ldr sp, =stack_svc_top
	msr cpsr_c, #0xd2
	ldr sp, =stack_irq_top
	msr cpsr_c, #0xd7
	ldr sp, =stack_abt_top
	msr cpsr_c, #0xdb
	ldr sp, =stack_und_top
	msr cpsr_c, #0xd3

	/*
	 * SCTLR.M, A, C, and I off. C runs with the MMU off, where every
	 * access is strongly ordered, and vm_on is what turns translation
	 * on. Caches stay off after that as well.
	 */
	mrc p15, 0, r0, c1, c0, 0
	bic r0, r0, #(1 << 0)
	bic r0, r0, #(1 << 1)
	bic r0, r0, #(1 << 2)
	bic r0, r0, #(1 << 12)
	mcr p15, 0, r0, c1, c0, 0
	isb

	/* VBAR is the base of the eight-word vector table. */
	ldr r0, =vector_table
	mcr p15, 0, r0, c12, c0, 0

	/*
	 * BSS holds the mode stacks and the zeroed globals. r2 is the
	 * device tree; main receives it in r0. A return from main is
	 * the shell's exit.
	 */
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
