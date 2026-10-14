/*
 * The kernel sets sp before sret. main returns here only if it falls
 * through; exit then ends the program.
 */
.section .text.init
.globl _start
_start:
	call main
	li a0, 0
	li a7, 0
	ecall
	j .
