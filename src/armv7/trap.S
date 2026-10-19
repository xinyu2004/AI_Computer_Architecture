/*
 * VBAR points here. SVC is the system call. IRQ is the timer and the
 * virtio completion. lr already addresses the next instruction after
 * svc. An IRQ return subtracts 4.
 */
.syntax unified
.arm

.section .text
.balign 32
.globl vector_table
vector_table:
	b .
	b und_entry
	b svc_entry
	b abt_entry
	b abt_entry
	b .
	b irq_entry
	b fiq_entry

und_entry:
	cpsid i
	bl fault_handler
	b .

abt_entry:
	cpsid i
	bl fault_handler
	b .

fiq_entry:
	b .

irq_entry:
	sub lr, lr, #4
	stmdb sp!, {r0-r3, r12, lr}
	bl irq_handler
	ldmia sp!, {r0-r3, r12, pc}^

svc_entry:
	cpsid i
	sub sp, sp, #68
	stmia sp, {r0-r12}
	cpsid i, #0x1f
	mov r0, sp
	mov r1, lr
	cpsid i, #0x13
	str r0, [sp, #52]
	str r1, [sp, #56]
	str lr, [sp, #60]
	mrs r0, spsr
	str r0, [sp, #64]
	mov r0, sp
	bl syscall
	cpsid i
	ldr r0, [sp, #64]
	msr spsr_cxsf, r0
	ldr r0, [sp, #52]
	ldr r1, [sp, #56]
	cpsid i, #0x1f
	mov sp, r0
	mov lr, r1
	cpsid i, #0x13
	ldr lr, [sp, #60]
	ldmia sp, {r0-r12}
	add sp, sp, #68
	movs pc, lr

.globl user_enter
user_enter:
	cpsid i, #0x1f
	mov sp, r1
	cpsid i, #0x13
	mov lr, r0
	mov r2, #0x10
	msr spsr_cxsf, r2
	movs pc, lr

.globl user_resume
user_resume:
	ldr r4, =child_tf
	ldr r0, [r4, #52]
	ldr r1, [r4, #56]
	cpsid i, #0x1f
	mov sp, r0
	mov lr, r1
	cpsid i, #0x13
	ldr r0, [r4, #64]
	msr spsr_cxsf, r0
	ldr lr, [r4, #60]
	ldmia r4, {r0-r12}
	movs pc, lr
