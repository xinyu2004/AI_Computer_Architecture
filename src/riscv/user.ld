/*
 * User programs for RV32 and RV64 are linked at 0x10000, the same
 * window both kernels map. The stack top the kernel installs is 0x81000.
 */
OUTPUT_ARCH("riscv")
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
		*(.sdata .sdata.*)
	}

	.bss : {
		*(.bss .bss.*)
		*(.sbss .sbss.*)
		*(COMMON)
	}
}
