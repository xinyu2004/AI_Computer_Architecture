/*
 * The kernel sets sp before returning to this address. main returns
 * here only if it falls through; exit then ends the program.
 */
.syntax unified
.arm

.section .text.init
.globl _start
_start:
	bl main
	mov r0, #0
	mov r7, #0
	svc #0
	b .
