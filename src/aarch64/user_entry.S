/*
 * The kernel sets sp before returning to this address. main returns
 * here only if it falls through; exit then ends the program.
 */
.section .text.init
.globl _start
_start:
	bl main
	mov x0, #0
	mov x8, #0
	svc #0
	b .
