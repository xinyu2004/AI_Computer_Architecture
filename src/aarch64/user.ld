/*
 * User programs are linked at 0x10000, the same window the kernel maps.
 * --nmagic keeps the file offset from being padded out to that address.
 * The stack pointer the kernel installs is 16-byte aligned.
 */
OUTPUT_ARCH(aarch64)
ENTRY(_start)

SECTIONS
{
	. = 0x10000;

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
		*(.bss .bss.*)
		*(COMMON)
	}

	/DISCARD/ : {
		*(.note*)
		*(.comment)
		*(.eh_frame*)
	}
}
