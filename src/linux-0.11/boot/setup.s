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

	cli

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
	lidt	idt_48
	lgdt	gdt_48

	call	empty_8042
	movb	$0xD1, %al
	outb	%al, $0x64
	call	empty_8042
	movb	$0xDF, %al
	outb	%al, $0x60
	call	empty_8042

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

	movw	$0x0001, %ax
	lmsw	%ax
	ljmp	$8, $0

empty_8042:
	.word	0x00eb,0x00eb
	inb	$0x64, %al
	testb	$2, %al
	jnz	empty_8042
	ret

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
