/*
 * OpenSBI, in M-mode, jumps here in S-mode with satp bare.
 * a0 is the hart id. a1 is the device-tree address.
 * QEMU's virt board loads this kernel at 0x80400000, after the firmware.
 */
.section .text.init
.globl _start
_start:
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
	call sbi_shutdown
