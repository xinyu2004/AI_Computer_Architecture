/*
 *  linux/mm/page.s
 *
 *  (C) 1991  Linus Torvalds
 */

/*
 * page.s contains the low-level page-exception code.
 * the real work is done in mm.c
 */

.globl _page_fault

/*
 * Page fault, vector 14. The CPU pushes an error code and then the
 * usual EFLAGS, CS, EIP. The error code's low bit is 0 when the
 * faulting translation was not present, and 1 when the page was
 * present but the access was not allowed (the write-protect case).
 * CR2 holds the linear address that faulted. Swapping EAX with the
 * error code puts that code where do_no_page and do_wp_page expect
 * it, and leaves EAX saved on the stack for IRET.
 */
_page_fault:
	xchgl %eax,(%esp)
	pushl %ecx
	pushl %edx
	push %ds
	push %es
	push %fs
	movl $0x10,%edx
	mov %dx,%ds
	mov %dx,%es
	mov %dx,%fs
	movl %cr2,%edx
	pushl %edx
	pushl %eax
	testl $1,%eax
	jne 1f
	call _do_no_page
	jmp 2f
1:	call _do_wp_page
2:	addl $8,%esp
	pop %fs
	pop %es
	pop %ds
	popl %edx
	popl %ecx
	popl %eax
	iret
