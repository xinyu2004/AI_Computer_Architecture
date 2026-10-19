/*
 * The virt stub places a raw Linux ARM image at 0x40010000 and jumps
 * there. Each CPU mode has its own stack in BSS: undefined and abort
 * are 1KiB, IRQ is 4KiB, SVC is 8KiB.
 */
OUTPUT_ARCH(arm)
ENTRY(_start)

SECTIONS
{
	. = 0x40010000;

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
		. = ALIGN(8);
		__bss_start = .;
		*(.bss .bss.*)
		*(COMMON)
		. = ALIGN(8);
		stack_und = .;
		. += 1024;
		stack_und_top = .;
		stack_abt = .;
		. += 1024;
		stack_abt_top = .;
		stack_irq = .;
		. += 4096;
		stack_irq_top = .;
		stack_svc = .;
		. += 8192;
		stack_svc_top = .;
		. = ALIGN(8);
		__bss_end = .;
	}

	_end = .;
}
