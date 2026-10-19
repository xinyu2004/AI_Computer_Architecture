/*
 * Supervisor trap vector. Hardware clears SIE, saves the old SIE in
 * SPIE, and puts the faulting instruction's address in sepc.
 *
 * Each saved register is 8 bytes. sscratch is 0 while the kernel is
 * running, and points at trapframe while a user program is running.
 */
.section .bss
.balign 16
.globl trapframe
trapframe:
	.space 280

.section .text
.globl trap_entry
.balign 4
trap_entry:
	/*
	 * Swap a0 with sscratch. Zero means the trap came from the kernel,
	 * and the old a0 is put back. A user trap leaves the trapframe
	 * pointer in a0 and branches to from_user.
	 */
	csrrw a0, sscratch, a0
	bnez a0, from_user
	csrrw a0, sscratch, a0

	/*
	 * Kernel frame, 280 bytes, eight bytes per register. x1 is at 8,
	 * sp at 16, sepc at 256, sstatus at 264. x0 is not stored.
	 * sscratch stays 0, so a nested trap takes this path again.
	 */
	addi sp, sp, -280
	sd t0, 40(sp)
	addi t0, sp, 280
	sd t0, 16(sp)
	sd ra, 8(sp)
	sd gp, 24(sp)
	sd tp, 32(sp)
	sd t1, 48(sp)
	sd t2, 56(sp)
	sd s0, 64(sp)
	sd s1, 72(sp)
	sd a0, 80(sp)
	sd a1, 88(sp)
	sd a2, 96(sp)
	sd a3, 104(sp)
	sd a4, 112(sp)
	sd a5, 120(sp)
	sd a6, 128(sp)
	sd a7, 136(sp)
	sd s2, 144(sp)
	sd s3, 152(sp)
	sd s4, 160(sp)
	sd s5, 168(sp)
	sd s6, 176(sp)
	sd s7, 184(sp)
	sd s8, 192(sp)
	sd s9, 200(sp)
	sd s10, 208(sp)
	sd s11, 216(sp)
	sd t3, 224(sp)
	sd t4, 232(sp)
	sd t5, 240(sp)
	sd t6, 248(sp)
	csrr t0, sepc
	sd t0, 256(sp)
	csrr t0, sstatus
	sd t0, 264(sp)
	mv a0, sp
	call trap_handler
	ld ra, 8(sp)
	ld gp, 24(sp)
	ld tp, 32(sp)
	ld t0, 40(sp)
	ld t1, 48(sp)
	ld t2, 56(sp)
	ld s0, 64(sp)
	ld s1, 72(sp)
	ld a0, 80(sp)
	ld a1, 88(sp)
	ld a2, 96(sp)
	ld a3, 104(sp)
	ld a4, 112(sp)
	ld a5, 120(sp)
	ld a6, 128(sp)
	ld a7, 136(sp)
	ld s2, 144(sp)
	ld s3, 152(sp)
	ld s4, 160(sp)
	ld s5, 168(sp)
	ld s6, 176(sp)
	ld s7, 184(sp)
	ld s8, 192(sp)
	ld s9, 200(sp)
	ld s10, 208(sp)
	ld s11, 216(sp)
	ld t3, 224(sp)
	ld t4, 232(sp)
	ld t5, 240(sp)
	ld t6, 248(sp)
	ld t0, 256(sp)
	csrw sepc, t0
	ld t0, 264(sp)
	csrw sstatus, t0
	addi sp, sp, 280
	sret

	/*
	 * User frame. a0 is the trapframe. The old a0 was left in sscratch
	 * and is written at offset 80. kernel_sp at offset 272 becomes sp
	 * for trap_handler. sscratch is then cleared so a nested trap takes
	 * the kernel path. sret restores sepc, sstatus, and the user registers.
	 */
from_user:
	sd ra, 8(a0)
	sd sp, 16(a0)
	sd gp, 24(a0)
	sd tp, 32(a0)
	sd t0, 40(a0)
	sd t1, 48(a0)
	sd t2, 56(a0)
	sd s0, 64(a0)
	sd s1, 72(a0)
	sd a1, 88(a0)
	sd a2, 96(a0)
	sd a3, 104(a0)
	sd a4, 112(a0)
	sd a5, 120(a0)
	sd a6, 128(a0)
	sd a7, 136(a0)
	sd s2, 144(a0)
	sd s3, 152(a0)
	sd s4, 160(a0)
	sd s5, 168(a0)
	sd s6, 176(a0)
	sd s7, 184(a0)
	sd s8, 192(a0)
	sd s9, 200(a0)
	sd s10, 208(a0)
	sd s11, 216(a0)
	sd t3, 224(a0)
	sd t4, 232(a0)
	sd t5, 240(a0)
	sd t6, 248(a0)
	csrr t0, sscratch
	sd t0, 80(a0)
	csrr t0, sepc
	sd t0, 256(a0)
	csrr t0, sstatus
	sd t0, 264(a0)
	ld sp, 272(a0)
	csrw sscratch, zero
	call trap_handler
	la a0, trapframe
	ld t0, 256(a0)
	csrw sepc, t0
	ld t0, 264(a0)
	csrw sstatus, t0
	csrw sscratch, a0
	ld ra, 8(a0)
	ld sp, 16(a0)
	ld gp, 24(a0)
	ld tp, 32(a0)
	ld t0, 40(a0)
	ld t1, 48(a0)
	ld t2, 56(a0)
	ld s0, 64(a0)
	ld s1, 72(a0)
	ld a1, 88(a0)
	ld a2, 96(a0)
	ld a3, 104(a0)
	ld a4, 112(a0)
	ld a5, 120(a0)
	ld a6, 128(a0)
	ld a7, 136(a0)
	ld s2, 144(a0)
	ld s3, 152(a0)
	ld s4, 160(a0)
	ld s5, 168(a0)
	ld s6, 176(a0)
	ld s7, 184(a0)
	ld s8, 192(a0)
	ld s9, 200(a0)
	ld s10, 208(a0)
	ld s11, 216(a0)
	ld t3, 224(a0)
	ld t4, 232(a0)
	ld t5, 240(a0)
	ld t6, 248(a0)
	ld a0, 80(a0)
	sret

/*
 * a0 is the first user instruction. a1 is the user stack.
 * SPP cleared means sret enters U-mode. SPIE set means that return
 * turns interrupts on, so the timer keeps ticking at the prompt.
 * SUM (bit 18) lets S-mode touch user pages while a syscall runs.
 */
.globl user_enter
user_enter:
	la t0, trapframe
	sd a1, 16(t0)
	sd sp, 272(t0)
	csrw sepc, a0
	li t1, (1 << 8)
	csrc sstatus, t1
	li t1, (1 << 5) | (1 << 18)
	csrs sstatus, t1
	csrw sscratch, t0
	mv sp, a1
	sret

.globl user_resume
user_resume:
	la a0, trapframe
	ld t0, 256(a0)
	csrw sepc, t0
	ld t0, 264(a0)
	csrw sstatus, t0
	csrw sscratch, a0
	ld ra, 8(a0)
	ld sp, 16(a0)
	ld gp, 24(a0)
	ld tp, 32(a0)
	ld t0, 40(a0)
	ld t1, 48(a0)
	ld t2, 56(a0)
	ld s0, 64(a0)
	ld s1, 72(a0)
	ld a1, 88(a0)
	ld a2, 96(a0)
	ld a3, 104(a0)
	ld a4, 112(a0)
	ld a5, 120(a0)
	ld a6, 128(a0)
	ld a7, 136(a0)
	ld s2, 144(a0)
	ld s3, 152(a0)
	ld s4, 160(a0)
	ld s5, 168(a0)
	ld s6, 176(a0)
	ld s7, 184(a0)
	ld s8, 192(a0)
	ld s9, 200(a0)
	ld s10, 208(a0)
	ld s11, 216(a0)
	ld t3, 224(a0)
	ld t4, 232(a0)
	ld t5, 240(a0)
	ld t6, 248(a0)
	ld a0, 80(a0)
	sret
