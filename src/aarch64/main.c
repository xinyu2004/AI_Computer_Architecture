#include "aarch64.h"

/*
 * The same shell as src/rv32, on Cortex-A57. The boot stub enters in
 * EL1h with the device tree in x0. Boot prints the machine lines, turns
 * a 4KiB page table on, and loads sh. The timer is already running at
 * the prompt.
 */

void main(uint64_t dtb)
{
	struct fdt_info fdt;
	uint64_t el;
	uint32_t freq;

	/*
	 * x0 was the device tree. A bad header stops here: there is no
	 * UART address to print with. PSCI is recorded before any print
	 * so a later exit can power the machine off.
	 */
	if (fdt_parse((const void *)dtb, &fdt) != 0) {
		for (;;)
			cpu_wfi();
	}
	poweroff_init(fdt.psci_hvc);

	/* Machine lines: CurrentEL, UART base, CNTFRQ_EL0. */
	uart_init(fdt.uart);
	uart_puts("aarch64\n");
	asm volatile("mrs %0, CurrentEL" : "=r"(el));
	uart_puts("currentel ");
	uart_hex((uint32_t)el);
	uart_putc('\n');
	uart_puts("uart ");
	uart_hex(fdt.uart);
	uart_putc('\n');
	freq = timer_freq();
	if (freq == 0)
		freq = fdt.timebase;
	uart_puts("timebase ");
	uart_u32(freq);
	uart_putc('\n');

	/*
	 * The GIC is programmed before the MMU, using the physical
	 * addresses from the tree. vm_on writes TTBR0_EL1 and sets
	 * SCTLR_EL1.M. shell_start brings up the disk and erets into sh.
	 * The printed timebase is CNTFRQ_EL0, which sleep uses as well.
	 */
	gic_init(fdt.gicd, fdt.gicc);
	timer_set_irq(fdt.timer_irq);
	vm_init(fdt.uart, fdt.virtio, fdt.nvirtio, fdt.gicd, fdt.gicc);
	vm_on();
	uart_puts("ttbr0 on\n");
	fdt.timebase = freq;
	shell_start(&fdt);
}
