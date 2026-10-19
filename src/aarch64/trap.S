/*
 * VBAR_EL1 points here. Each slot is 128 bytes. svc from EL0 is the
 * lower-EL AArch64 sync slot. ELR_EL1 already points at the instruction
 * after svc. An IRQ returns to ELR as it stands. The same IRQ entry
 * serves EL1 and EL0.
 */
.macro ventry label
.balign 128
	b \label
.endm

	/*
	 * Sixteen slots, four groups of four: current EL with SP0, current
	 * EL with SPx, lower EL AArch64, lower EL AArch32. Inside a group
	 * the order is sync, IRQ, FIQ, SError. Slot 5 (offset 0x280) is an
	 * IRQ taken in the kernel. Slot 8 (offset 0x400) is svc from EL0.
	 * Slot 9 (offset 0x480) is an IRQ taken from EL0. VBAR_EL1 requires
	 * 2KiB alignment.
	 */
.section .text
.balign 2048
.globl vector_table
vector_table:
	ventry fault_entry
	ventry fault_entry
	ventry fault_entry
	ventry fault_entry
	ventry fault_entry
	ventry irq_entry
	ventry fault_entry
	ventry fault_entry
	ventry svc_entry
	ventry irq_entry
	ventry fault_entry
	ventry fault_entry
	ventry fault_entry
	ventry fault_entry
	ventry fault_entry
	ventry fault_entry

fault_entry:
	bl fault_handler
	b .

	/*
	 * 192 bytes of caller-saved registers on the current stack. ELR_EL1
	 * already points where eret should return, so it is left unchanged.
	 */
irq_entry:
	sub sp, sp, #192
	stp x0, x1, [sp, #0]
	stp x2, x3, [sp, #16]
	stp x4, x5, [sp, #32]
	stp x6, x7, [sp, #48]
	stp x8, x9, [sp, #64]
	stp x10, x11, [sp, #80]
	stp x12, x13, [sp, #96]
	stp x14, x15, [sp, #112]
	stp x16, x17, [sp, #128]
	stp x18, x29, [sp, #144]
	str x30, [sp, #160]
	bl irq_handler
	ldr x30, [sp, #160]
	ldp x18, x29, [sp, #144]
	ldp x16, x17, [sp, #128]
	ldp x14, x15, [sp, #112]
	ldp x12, x13, [sp, #96]
	ldp x10, x11, [sp, #80]
	ldp x8, x9, [sp, #64]
	ldp x6, x7, [sp, #48]
	ldp x4, x5, [sp, #32]
	ldp x2, x3, [sp, #16]
	ldp x0, x1, [sp, #0]
	add sp, sp, #192
	eret

	/*
	 * 272-byte frame: x0-x30, then SP_EL0 at 248, ELR_EL1 at 256, and
	 * SPSR_EL1 at 264. ESR_EL1 bits [31:26] are the exception class.
	 * 0x15 is an svc issued from AArch64. ELR_EL1 already points at
	 * the instruction after svc, so it is stored as it stands.
	 */
svc_entry:
	sub sp, sp, #272
	stp x0, x1, [sp, #0]
	stp x2, x3, [sp, #16]
	stp x4, x5, [sp, #32]
	stp x6, x7, [sp, #48]
	stp x8, x9, [sp, #64]
	stp x10, x11, [sp, #80]
	stp x12, x13, [sp, #96]
	stp x14, x15, [sp, #112]
	stp x16, x17, [sp, #128]
	stp x18, x19, [sp, #144]
	stp x20, x21, [sp, #160]
	stp x22, x23, [sp, #176]
	stp x24, x25, [sp, #192]
	stp x26, x27, [sp, #208]
	stp x28, x29, [sp, #224]
	str x30, [sp, #240]
	mrs x9, sp_el0
	str x9, [sp, #248]
	mrs x9, elr_el1
	str x9, [sp, #256]
	mrs x9, spsr_el1
	str x9, [sp, #264]
	mrs x9, esr_el1
	lsr x9, x9, #26
	cmp x9, #0x15
	b.ne fault_entry
	mov x0, sp
	bl syscall
	ldr x9, [sp, #264]
	msr spsr_el1, x9
	ldr x9, [sp, #256]
	msr elr_el1, x9
	ldr x9, [sp, #248]
	msr sp_el0, x9
	ldr x30, [sp, #240]
	ldp x28, x29, [sp, #224]
	ldp x26, x27, [sp, #208]
	ldp x24, x25, [sp, #192]
	ldp x22, x23, [sp, #176]
	ldp x20, x21, [sp, #160]
	ldp x18, x19, [sp, #144]
	ldp x16, x17, [sp, #128]
	ldp x14, x15, [sp, #112]
	ldp x12, x13, [sp, #96]
	ldp x10, x11, [sp, #80]
	ldp x8, x9, [sp, #64]
	ldp x6, x7, [sp, #48]
	ldp x4, x5, [sp, #32]
	ldp x2, x3, [sp, #16]
	ldp x0, x1, [sp, #0]
	add sp, sp, #272
	eret

	/*
	 * x0 is the first user instruction, x1 is the user stack. SPSR_EL1
	 * zero is EL0t. eret copies it into PSTATE and jumps to ELR_EL1.
	 */
.globl user_enter
user_enter:
	msr sp_el0, x1
	msr elr_el1, x0
	msr spsr_el1, xzr
	eret

	/*
	 * child_tf holds the program started by exec. SP_EL0 is at 248,
	 * ELR_EL1 at 256, SPSR_EL1 at 264. x9 is the pointer into the frame
	 * and is cleared before eret so the user sees it as zero.
	 */
.globl user_resume
user_resume:
	ldr x9, =child_tf
	ldr x0, [x9, #248]
	msr sp_el0, x0
	ldr x0, [x9, #256]
	msr elr_el1, x0
	ldr x0, [x9, #264]
	msr spsr_el1, x0
	ldp x2, x3, [x9, #16]
	ldp x4, x5, [x9, #32]
	ldp x6, x7, [x9, #48]
	ldp x10, x11, [x9, #80]
	ldp x12, x13, [x9, #96]
	ldp x14, x15, [x9, #112]
	ldp x16, x17, [x9, #128]
	ldp x18, x19, [x9, #144]
	ldp x20, x21, [x9, #160]
	ldp x22, x23, [x9, #176]
	ldp x24, x25, [x9, #192]
	ldp x26, x27, [x9, #208]
	ldp x28, x29, [x9, #224]
	ldr x30, [x9, #240]
	ldp x0, x1, [x9, #0]
	ldr x8, [x9, #64]
	mov x9, #0
	eret
