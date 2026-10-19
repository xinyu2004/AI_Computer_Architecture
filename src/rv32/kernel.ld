/*
 * Linked at the start of QEMU virt RAM. The kernel stack is 8KiB at
 * the end of BSS. _start is the first instruction.
 */
OUTPUT_ARCH("riscv")
ENTRY(_start)

SECTIONS
{
	. = 0x80000000;

	.text : {
		*(.text.init)
		*(.text .text.*)
	}

	.rodata : {
		*(.rodata .rodata.*)
	}

	.data : {
		*(.data .data.*)
		*(.sdata .sdata.*)
	}

	.bss : {
		. = ALIGN(4);
		__bss_start = .;
		*(.bss .bss.*)
		*(.sbss .sbss.*)
		*(COMMON)
		. = ALIGN(16);
		stack = .;
		. += 8192;
		stack_top = .;
		. = ALIGN(4);
		__bss_end = .;
	}

	_end = .;
}
