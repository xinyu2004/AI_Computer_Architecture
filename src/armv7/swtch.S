/*
 * void swtch(struct context *old, struct context *next);
 *
 * Save the return address, the stack pointer, and the callee-saved
 * registers, then load the other task's. Both tasks stay in SVC.
 */
.syntax unified
.arm

.globl swtch
swtch:
	str lr, [r0, #32]
	str sp, [r0, #36]
	stmia r0, {r4-r11}
	ldr lr, [r1, #32]
	ldr sp, [r1, #36]
	ldmia r1, {r4-r11}
	bx lr
