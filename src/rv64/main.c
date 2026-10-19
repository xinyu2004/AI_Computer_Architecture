#include "rv64.h"

/*
 * The same shell as src/rv32, on 64-bit registers. _start begins in
 * M-mode and mrets to S-mode. Boot prints the machine lines, turns an
 * Sv39 page table on, and loads sh. The timer is already running at
 * the prompt.
 */

void main(uint64_t hart, uint64_t dtb)
{
	struct fdt_info fdt;

	if (fdt_parse((const void *)(uintptr_t)dtb, &fdt) != 0) {
		for (;;)
			asm volatile("wfi");
	}
	poweroff_at(fdt.test);

	uart_init(fdt.uart);
	uart_puts("rv64\n");
	uart_puts("hart ");
	uart_u32((uint32_t)hart);
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
	uart_puts("sv39 on\n");
	shell_start(&fdt);
}
