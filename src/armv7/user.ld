/*
 * User programs are linked at 0x10000, the same window the kernel maps.
 * --nmagic keeps the file offset from being padded out to that address.
 */
OUTPUT_ARCH(arm)
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
}
