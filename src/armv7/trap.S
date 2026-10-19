/*
 * VBAR points here. SVC is the system call. IRQ is the timer and the
 * virtio completion. lr already addresses the next instruction after
 * svc. An IRQ return subtracts 4.
 */
.syntax unified
.arm

.section .text
	/*
	 * Eight words, in the order the CPU uses: reset, undefined, SVC,
	 * prefetch abort, data abort, reserved, IRQ, FIQ. Reset and FIQ
	 * are not taken on this board. The table is 32-byte aligned because
	 * that is what VBAR requires.
	 */
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

	/*
	 * Hardware leaves LR_irq four bytes past the interrupted
	 * instruction. Subtract 4, save the caller-saved registers on the
	 * IRQ stack, and return with pc and the caret so SPSR is copied
	 * back into CPSR.
	 */
irq_entry:
	sub lr, lr, #4
	stmdb sp!, {r0-r3, r12, lr}
	bl irq_handler
	ldmia sp!, {r0-r3, r12, pc}^

	/*
	 * Hardware leaves LR_svc at the instruction after svc, so this
	 * path does not adjust it. The 68-byte frame is r0-r12, then the
	 * user sp and lr (read by switching to System, mode 0x1f), then
	 * LR_svc as the return pc, then SPSR. syscall sees that frame.
	 */
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

	/*
	 * r0 is the first user instruction, r1 is the user stack. System
	 * mode shares the user bank of sp and lr, so those are written
	 * there. SPSR 0x10 is user mode. movs pc, lr copies SPSR into CPSR
	 * and jumps to the program.
	 */
.globl user_enter
user_enter:
	cpsid i, #0x1f
	mov sp, r1
	cpsid i, #0x13
	mov lr, r0
	mov r2, #0x10
	msr spsr_cxsf, r2
	movs pc, lr

	/*
	 * child_tf holds the program started by exec. Offsets match the
	 * trapframe: user sp at 52, user lr at 56, return pc at 60, SPSR
	 * at 64. System mode receives the user bank, then movs returns.
	 */
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
