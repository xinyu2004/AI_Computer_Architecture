/*
 *	bootsect.s		(C) 1991 Linus Torvalds
 *
 * GNU as translation. Load addresses match the 1991 as86 source:
 * BIOS enters at 0x7C00, this sector moves to 0x90000, setup follows,
 * and the kernel is loaded at 0x10000.
 */
.code16
.text

SETUPLEN = 4
BOOTSEG  = 0x07c0
INITSEG  = 0x9000
SETUPSEG = 0x9020
SYSSEG   = 0x1000
SYSSIZE  = 0x3000
ENDSEG   = SYSSEG + SYSSIZE

/* 0x306 = major 3, minor 6 (/dev/hd6). tools/build overwrites this. */
ROOT_DEV = 0x306

/*
 * BIOS enters here in real mode, CS:IP = 0000:7C00. A segment
 * address is the linear address shifted right by 4, so BOOTSEG
 * 0x07C0 is linear 0x7C00. The first act is to copy this 512-byte
 * sector to 0x90000 and jump there, which frees 0x7C00 for the
 * kernel load that follows.
 */
.global start
start:
	movw	$BOOTSEG, %ax
	movw	%ax, %ds
	movw	$INITSEG, %ax
	movw	%ax, %es
	movw	$256, %cx
	subw	%si, %si
	subw	%di, %di
	rep; movsw		/* repeat a word copy CX times */
	ljmp	$INITSEG, $go	/* far jump: reload CS and IP together */

go:
	movw	%cs, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %ss
	movw	$0xFF00, %sp

/*
 * INT 0x13 reads the floppy. AH=2 is "read sectors". Setup is the
 * next four sectors and lands at 0x90200. The kernel image is then
 * read to 0x10000 (SYSSEG). setup.s moves that image down to
 * physical 0 after this sector has finished with the BIOS.
 */
load_setup:
	movw	$0x0000, %dx
	movw	$0x0002, %cx
	movw	$0x0200, %bx
	movw	$0x0200+SETUPLEN, %ax
	int	$0x13
	jnc	ok_load_setup
	movw	$0x0000, %dx
	movw	$0x0000, %ax
	int	$0x13
	jmp	load_setup

ok_load_setup:
	movb	$0x00, %dl
	movw	$0x0800, %ax
	int	$0x13
	movb	$0x00, %ch
	movw	%cx, %cs:sectors
	movw	$INITSEG, %ax
	movw	%ax, %es

	movb	$0x03, %ah
	xorb	%bh, %bh
	int	$0x10

	movw	$24, %cx
	movw	$0x0007, %bx
	movw	$msg1, %bp
	movw	$0x1301, %ax
	int	$0x10

	movw	$SYSSEG, %ax
	movw	%ax, %es
	call	read_it
	call	kill_motor

	movw	%cs:root_dev, %ax
	cmpw	$0, %ax
	jne	root_defined
	movw	%cs:sectors, %bx
	movw	$0x0208, %ax		/* /dev/PS0 - 1.2Mb */
	cmpw	$15, %bx
	je	root_defined
	movw	$0x021c, %ax		/* /dev/PS0 - 1.44Mb */
	cmpw	$18, %bx
	je	root_defined
undef_root:
	jmp	undef_root
root_defined:
	movw	%ax, %cs:root_dev
	ljmp	$SETUPSEG, $0

sread:	.word 1+SETUPLEN
head:	.word 0
track:	.word 0

read_it:
	movw	%es, %ax
	testw	$0x0fff, %ax
die:	jne	die
	xorw	%bx, %bx
rp_read:
	movw	%es, %ax
	cmpw	$ENDSEG, %ax
	jb	ok1_read
	ret
ok1_read:
	movw	%cs:sectors, %ax
	subw	sread, %ax
	movw	%ax, %cx
	shlw	$9, %cx
	addw	%bx, %cx
	jnc	ok2_read
	je	ok2_read
	xorw	%ax, %ax
	subw	%bx, %ax
	shrw	$9, %ax
ok2_read:
	call	read_track
	movw	%ax, %cx
	addw	sread, %ax
	cmpw	%cs:sectors, %ax
	jne	ok3_read
	movw	$1, %ax
	subw	head, %ax
	jne	ok4_read
	incw	track
ok4_read:
	movw	%ax, head
	xorw	%ax, %ax
ok3_read:
	movw	%ax, sread
	shlw	$9, %cx
	addw	%cx, %bx
	jnc	rp_read
	movw	%es, %ax
	addw	$0x1000, %ax
	movw	%ax, %es
	xorw	%bx, %bx
	jmp	rp_read

read_track:
	pushw	%ax
	pushw	%bx
	pushw	%cx
	pushw	%dx
	movw	track, %dx
	movw	sread, %cx
	incw	%cx
	movb	%dl, %ch
	movw	head, %dx
	movb	%dl, %dh
	movb	$0, %dl
	andw	$0x0100, %dx
	movb	$2, %ah
	int	$0x13
	jc	bad_rt
	popw	%dx
	popw	%cx
	popw	%bx
	popw	%ax
	ret
bad_rt:
	movw	$0, %ax
	movw	$0, %dx
	int	$0x13
	popw	%dx
	popw	%cx
	popw	%bx
	popw	%ax
	jmp	read_track

kill_motor:
	pushw	%dx
	movw	$0x3f2, %dx
	xorb	%al, %al
	outb	%al, %dx
	popw	%dx
	ret

sectors:
	.word 0

msg1:
	.byte 13,10
	.ascii "Loading system ..."
	.byte 13,10,13,10

	.org 508
root_dev:
	.word ROOT_DEV
boot_flag:
	.word 0xAA55
