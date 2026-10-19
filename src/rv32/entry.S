/*
 * The CPU resets in M-mode. a0 is the hart id. a1 is the device tree.
 * QEMU's virt board starts RAM at 0x80000000, which is where this kernel
 * is linked. PMP would otherwise deny S-mode. mret drops into S-mode.
 */
.section .text.init
.globl _start
_start:
	/*
	 * pmpaddr0 all ones, with NAPOT and read/write/execute in pmpcfg0,
	 * opens every physical address. The mret below would fault on the
	 * first S-mode fetch without this entry.
	 */
	li t0, -1
	csrw pmpaddr0, t0
	li t0, 0x1f
	csrw pmpcfg0, t0

	/*
	 * medeleg 0xfdff sends every exception except an ecall from M-mode
	 * (bit 11) to S-mode. mideleg 0x222 sends supervisor software,
	 * timer, and external interrupts (bits 1, 5, and 9).
	 */
	li t0, 0xfdff
	csrw medeleg, t0
	li t0, 0x222
	csrw mideleg, t0

	# Sstc: S-mode may use stimecmp only after STCE is set. On RV32
	# that bit is bit 31 of menvcfgh (CSR 0x31a). The time register
	# needs the TM bit in mcounteren as well.
	li t0, 0x80000000
	csrw 0x31a, t0
	li t0, 2
	csrw mcounteren, t0

	/*
	 * MPP is mstatus bits 12:11. Clear them, then set bit 11 so the
	 * previous privilege is S. mepc is s_mode. mret leaves M-mode and
	 * continues there. a0 is still the hart id and a1 is still the
	 * device tree.
	 */
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
	/*
	 * One S-mode stack, placed in BSS by the linker. s0 and s1 hold
	 * a0 and a1 across the zeroing of BSS, then main receives them.
	 * A return from main is the shell's exit.
	 */
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
