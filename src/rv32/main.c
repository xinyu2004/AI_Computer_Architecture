#include "rv32.h"

/*
 * A short RV32 kernel for the same five jobs as the 80386 path:
 * boot, page tables, a timer interrupt, a task switch, and one page copy.
 * _start begins in M-mode and mrets to S-mode. Boot prints the machine
 * lines, turns page tables on, and loads sh. The shell runs programs
 * from the disk. The timer is already running at the prompt.
 */

void main(uint32_t hart, uint32_t dtb)
{
	struct fdt_info fdt;

	if (fdt_parse((const void *)dtb, &fdt) != 0) {
		for (;;)
			asm volatile("wfi");
	}
	poweroff_at(fdt.test);

	uart_init(fdt.uart);
	uart_puts("rv32\n");
	uart_puts("hart ");
	uart_u32(hart);
	uart_putc('\n');
	uart_puts("uart ");
	uart_hex(fdt.uart);
	uart_putc('\n');
	uart_puts("timebase ");
	uart_u32(fdt.timebase);
	uart_putc('\n');

	trap_init();
	vm_init(fdt.uart, fdt.virtio, fdt.nvirtio, fdt.test, fdt.plic);
	vm_on();
	uart_puts("sv32 on\n");
	shell_start(&fdt);
}
