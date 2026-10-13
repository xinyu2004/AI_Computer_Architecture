OUTPUT_ARCH("riscv")
ENTRY(_start)

SECTIONS
{
	. = 0x80400000;

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
		. += 4096;
		stack_top = .;
		. = ALIGN(4);
		__bss_end = .;
	}

	_end = .;
}
