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
