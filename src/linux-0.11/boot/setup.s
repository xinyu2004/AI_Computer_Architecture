/*
 *	setup.s		(C) 1991 Linus Torvalds
 *
 * GNU as translation. This still asks the BIOS for memory and disk
 * parameters, parks them at 0x90000, and jumps to protected mode at 0.
 */
.code16
.text

INITSEG  = 0x9000
SETUPSEG = 0x9020

/*
 * Still real mode, running at 0x90200. DS is 0x9000, so the stores
 * below are the boot parameters at linear 0x90000: cursor position
 * at offset 0, kilobytes of memory above 1MB at offset 2, video
 * mode and columns at offsets 4 and 6. main() reads those fields
 * back before it sizes the kernel heap.
 */
.global start
start:
	movw	$INITSEG, %ax
	movw	%ax, %ds
	movb	$0x03, %ah
	xorb	%bh, %bh
	int	$0x10
	movw	%dx, 0

	movb	$0x88, %ah
	int	$0x15
	movw	%ax, 2

	movb	$0x0f, %ah
	int	$0x10
	movw	%bx, 4
	movw	%ax, 6

	movb	$0x12, %ah
	movb	$0x10, %bl
	int	$0x10
	movw	%ax, 8
	movw	%bx, 10
	movw	%cx, 12

	movw	$0x0000, %ax
	movw	%ax, %ds
	lds	4*0x41, %si
	movw	$INITSEG, %ax
	movw	%ax, %es
	movw	$0x0080, %di
	movw	$0x10, %cx
	rep; movsb

	movw	$0x0000, %ax
	movw	%ax, %ds
	lds	4*0x46, %si
	movw	$INITSEG, %ax
	movw	%ax, %es
	movw	$0x0090, %di
	movw	$0x10, %cx
	rep; movsb

	movw	$0x1500, %ax
	movb	$0x81, %dl
	int	$0x13
	jc	no_disk1
	cmpb	$3, %ah
	je	is_disk1
no_disk1:
	movw	$INITSEG, %ax
	movw	%ax, %es
	movw	$0x0090, %di
	movw	$0x10, %cx
	movw	$0x00, %ax
	rep; stosb
is_disk1:

	cli			/* Clear interrupt flag: maskable IRQs stay pending */

/*
 * Copy the kernel from 0x10000 down to physical 0, 64KB at a time.
 * This sector itself stays at 0x90200, so it is not overwritten.
 * After the copy, the 32-bit image begins at the address head.s
 * will execute.
 */
	movw	$0x0000, %ax
	cld
do_move:
	movw	%ax, %es
	addw	$0x1000, %ax
	cmpw	$0x9000, %ax
	jz	end_move
	movw	%ax, %ds
	subw	%di, %di
	subw	%si, %si
	movw	$0x8000, %cx
	rep; movsw
	jmp	do_move

end_move:
	movw	$SETUPSEG, %ax
	movw	%ax, %ds
	lidt	idt_48		/* Load IDT register: limit and base of the interrupt table */
	lgdt	gdt_48		/* Load GDT register: limit and base of the global table */

/*
 * Ask the keyboard controller to set A20. With A20 off, bit 20 of
 * a linear address wraps, so physical 0 and 1MB are the same cell
 * and the kernel's memory above 1MB cannot be addressed. OUTB writes
 * one byte to an I/O port; INB reads one back. 0xD1 tells the 8042
 * that the next byte is written to its output port; 0xDF turns A20
 * on. head.s checks the result before it trusts RAM.
 */
	call	empty_8042
	movb	$0xD1, %al
	outb	%al, $0x64
	call	empty_8042
	movb	$0xDF, %al
	outb	%al, $0x60
	call	empty_8042

/*
 * Reprogram both 8259 interrupt controllers. The PC BIOS leaves
 * hardware IRQs on vectors 0x08-0x0F, which collide with the CPU's
 * own exceptions. ICW1 (0x11) starts the sequence. The next bytes
 * are the new vector bases, 0x20 for the master and 0x28 for the
 * slave, so IRQ0 (the timer) becomes interrupt 0x20. Both masks are
 * then set to 0xFF: every IRQ is blocked until the kernel opens the
 * ones it handles. The 0x00EB words are short jumps used as a delay
 * between the port writes.
 */
	movb	$0x11, %al
	outb	%al, $0x20
	.word	0x00eb,0x00eb
	outb	%al, $0xA0
	.word	0x00eb,0x00eb
	movb	$0x20, %al
	outb	%al, $0x21
	.word	0x00eb,0x00eb
	movb	$0x28, %al
	outb	%al, $0xA1
	.word	0x00eb,0x00eb
	movb	$0x04, %al
	outb	%al, $0x21
	.word	0x00eb,0x00eb
	movb	$0x02, %al
	outb	%al, $0xA1
	.word	0x00eb,0x00eb
	movb	$0x01, %al
	outb	%al, $0x21
	.word	0x00eb,0x00eb
	outb	%al, $0xA1
	.word	0x00eb,0x00eb
	movb	$0xFF, %al
	outb	%al, $0x21
	.word	0x00eb,0x00eb
	outb	%al, $0xA1

/*
 * Enter protected mode. LMSW, Load Machine Status Word, writes the
 * low 16 bits of CR0. AX is 1, so only PE (bit 0) is set; paging
 * stays off. A 286-era LMSW cannot clear PE. The far jump loads CS
 * from selector 8, which is GDT index 1, and sets IP to 0. A far
 * jump is required: it reloads the segment descriptor cache and
 * empties the prefetch queue, so the following bytes are decoded as
 * 32-bit instructions. Those bytes are the kernel image now sitting
 * at physical 0.
 */
	movw	$0x0001, %ax
	lmsw	%ax		/* PE on; the low half of CR0 */
	ljmp	$8, $0

empty_8042:
	.word	0x00eb,0x00eb
	inb	$0x64, %al
	testb	$2, %al
	jnz	empty_8042
	ret

/*
 * Temporary GDT, used only for the jump into head.s. The null
 * descriptor is required at index 0. The code descriptor (selector 8)
 * and the data descriptor (selector 0x10) are flat: base 0, 32-bit,
 * 4KB granularity. The limit field 0x07FF with that granularity
 * covers 8MB. Type 0x9A is a readable code segment, DPL 0, present.
 * Type 0x92 is a writable data segment. head.s replaces this table
 * with the kernel GDT, whose limit covers 16MB.
 */
gdt:
	.word	0,0,0,0

	.word	0x07FF		/* 8Mb */
	.word	0x0000
	.word	0x9A00
	.word	0x00C0

	.word	0x07FF		/* 8Mb */
	.word	0x0000
	.word	0x9200
	.word	0x00C0

idt_48:
	.word	0
	.word	0,0

gdt_48:
	.word	0x800
	.word	512+gdt,0x9
