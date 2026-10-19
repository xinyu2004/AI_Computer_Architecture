/*
 * text_offset in the Image header is 0x80000, so this kernel is linked
 * at 0x40080000. One 8KiB EL1 stack sits in BSS, aligned to 16 bytes.
 */
OUTPUT_ARCH(aarch64)
ENTRY(_start)

SECTIONS
{
	. = 0x40080000;

	.text : {
		*(.text.init)
		*(.text .text.*)
	}

	.rodata : {
		*(.rodata .rodata.*)
	}

	.data : {
		*(.data .data.*)
	}

	.bss : {
		. = ALIGN(16);
		__bss_start = .;
		*(.bss .bss.*)
		*(COMMON)
		. = ALIGN(16);
		stack = .;
		. += 8192;
		stack_top = .;
		. = ALIGN(16);
		__bss_end = .;
	}

	_end = .;

	/DISCARD/ : {
		*(.note*)
		*(.comment)
		*(.eh_frame*)
	}
}
