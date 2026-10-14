/*
 * Supervisor trap vector. Hardware clears SIE, saves the old SIE in
 * SPIE, and puts the faulting instruction's address in sepc.
 *
 * sscratch is 0 while the kernel is running, and points at trapframe
 * while a user program is running. A user trap saves into that frame
 * and switches to the kernel stack recorded there. A kernel trap,
 * including a timer tick taken while the kernel is running, keeps
 * the current stack.
 */
.section .bss
.balign 16
.globl trapframe
trapframe:
	.space 144

.section .text
.globl trap_entry
.balign 4
trap_entry:
	csrrw a0, sscratch, a0
	bnez a0, from_user
	csrrw a0, sscratch, a0

	addi sp, sp, -144
	sw t0, 20(sp)
	addi t0, sp, 144
	sw t0, 8(sp)
	sw ra, 4(sp)
	sw gp, 12(sp)
	sw tp, 16(sp)
	sw t1, 24(sp)
	sw t2, 28(sp)
	sw s0, 32(sp)
	sw s1, 36(sp)
	sw a0, 40(sp)
	sw a1, 44(sp)
	sw a2, 48(sp)
	sw a3, 52(sp)
	sw a4, 56(sp)
	sw a5, 60(sp)
	sw a6, 64(sp)
	sw a7, 68(sp)
	sw s2, 72(sp)
	sw s3, 76(sp)
	sw s4, 80(sp)
	sw s5, 84(sp)
	sw s6, 88(sp)
	sw s7, 92(sp)
	sw s8, 96(sp)
	sw s9, 100(sp)
	sw s10, 104(sp)
	sw s11, 108(sp)
	sw t3, 112(sp)
	sw t4, 116(sp)
	sw t5, 120(sp)
	sw t6, 124(sp)
	csrr t0, sepc
	sw t0, 128(sp)
	csrr t0, sstatus
	sw t0, 132(sp)
	mv a0, sp
	call trap_handler
	lw ra, 4(sp)
	lw gp, 12(sp)
	lw tp, 16(sp)
	lw t0, 20(sp)
	lw t1, 24(sp)
	lw t2, 28(sp)
	lw s0, 32(sp)
	lw s1, 36(sp)
	lw a0, 40(sp)
	lw a1, 44(sp)
	lw a2, 48(sp)
	lw a3, 52(sp)
	lw a4, 56(sp)
	lw a5, 60(sp)
	lw a6, 64(sp)
	lw a7, 68(sp)
	lw s2, 72(sp)
	lw s3, 76(sp)
	lw s4, 80(sp)
	lw s5, 84(sp)
	lw s6, 88(sp)
	lw s7, 92(sp)
	lw s8, 96(sp)
	lw s9, 100(sp)
	lw s10, 104(sp)
	lw s11, 108(sp)
	lw t3, 112(sp)
	lw t4, 116(sp)
	lw t5, 120(sp)
	lw t6, 124(sp)
	lw t0, 128(sp)
	csrw sepc, t0
	lw t0, 132(sp)
	csrw sstatus, t0
	addi sp, sp, 144
	sret

from_user:
	sw ra, 4(a0)
	sw sp, 8(a0)
	sw gp, 12(a0)
	sw tp, 16(a0)
	sw t0, 20(a0)
	sw t1, 24(a0)
	sw t2, 28(a0)
	sw s0, 32(a0)
	sw s1, 36(a0)
	sw a1, 44(a0)
	sw a2, 48(a0)
	sw a3, 52(a0)
	sw a4, 56(a0)
	sw a5, 60(a0)
	sw a6, 64(a0)
	sw a7, 68(a0)
	sw s2, 72(a0)
	sw s3, 76(a0)
	sw s4, 80(a0)
	sw s5, 84(a0)
	sw s6, 88(a0)
	sw s7, 92(a0)
	sw s8, 96(a0)
	sw s9, 100(a0)
	sw s10, 104(a0)
	sw s11, 108(a0)
	sw t3, 112(a0)
	sw t4, 116(a0)
	sw t5, 120(a0)
	sw t6, 124(a0)
	csrr t0, sscratch
	sw t0, 40(a0)
	csrr t0, sepc
	sw t0, 128(a0)
	csrr t0, sstatus
	sw t0, 132(a0)
	lw sp, 136(a0)
	csrw sscratch, zero
	call trap_handler
	la a0, trapframe
	lw t0, 128(a0)
	csrw sepc, t0
	lw t0, 132(a0)
	csrw sstatus, t0
	csrw sscratch, a0
	lw ra, 4(a0)
	lw sp, 8(a0)
	lw gp, 12(a0)
	lw tp, 16(a0)
	lw t0, 20(a0)
	lw t1, 24(a0)
	lw t2, 28(a0)
	lw s0, 32(a0)
	lw s1, 36(a0)
	lw a1, 44(a0)
	lw a2, 48(a0)
	lw a3, 52(a0)
	lw a4, 56(a0)
	lw a5, 60(a0)
	lw a6, 64(a0)
	lw a7, 68(a0)
	lw s2, 72(a0)
	lw s3, 76(a0)
	lw s4, 80(a0)
	lw s5, 84(a0)
	lw s6, 88(a0)
	lw s7, 92(a0)
	lw s8, 96(a0)
	lw s9, 100(a0)
	lw s10, 104(a0)
	lw s11, 108(a0)
	lw t3, 112(a0)
	lw t4, 116(a0)
	lw t5, 120(a0)
	lw t6, 124(a0)
	lw a0, 40(a0)
	sret

/*
 * a0 is the first user instruction. a1 is the user stack.
 * SPP cleared means sret enters U-mode. SPIE cleared means that
 * return leaves interrupts off until a timer command turns them on.
 */
.globl user_enter
user_enter:
	la t0, trapframe
	sw a1, 8(t0)
	sw sp, 136(t0)
	csrw sepc, a0
	li t1, (1 << 8) | (1 << 5)
	csrc sstatus, t1
	li t1, (1 << 18)
	csrs sstatus, t1
	csrw sscratch, t0
	mv sp, a1
	sret
