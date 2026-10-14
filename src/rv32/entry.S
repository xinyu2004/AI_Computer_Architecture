/*
 * The CPU resets in M-mode. a0 is the hart id. a1 is the device tree.
 * QEMU's virt board starts RAM at 0x80000000, which is where this kernel
 * is linked. PMP would otherwise deny S-mode. mret drops into S-mode.
 */
.section .text.init
.globl _start
_start:
	li t0, -1
	csrw pmpaddr0, t0
	li t0, 0x1f
	csrw pmpcfg0, t0

	li t0, 0xfdff
	csrw medeleg, t0
	li t0, 0x222
	csrw mideleg, t0

	# Sstc: S-mode may use stimecmp only after STCE is set. The time
	# register needs the TM bit in mcounteren as well.
	li t0, 0x80000000
	csrw 0x31a, t0
	li t0, 2
	csrw mcounteren, t0

	csrr t0, mstatus
	li t1, ~0x1800
	and t0, t0, t1
	li t1, 0x800
	or t0, t0, t1
	csrw mstatus, t0

	la t0, s_mode
	csrw mepc, t0
	mret

s_mode:
	la sp, stack_top
	mv s0, a0
	mv s1, a1
	la t0, __bss_start
	la t1, __bss_end
1:
	bgeu t0, t1, 2f
	sw zero, 0(t0)
	addi t0, t0, 4
	j 1b
2:
	mv a0, s0
	mv a1, s1
	call main
	call poweroff
