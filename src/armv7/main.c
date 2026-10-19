#include "armv7.h"

/*
 * The same shell as src/rv32, on Cortex-A15. The boot stub enters in
 * SVC. Boot prints the machine lines, turns a short-descriptor page
 * table on, and loads sh. The timer is already running at the prompt.
 */

void main(uint32_t dtb)
{
	struct fdt_info fdt;
	uint32_t cpsr;
	uint32_t freq;

	/*
	 * r0 was the device tree from r2. A bad header stops here: there
	 * is no UART address to print with. PSCI is recorded before any
	 * print so a later exit can power the machine off.
	 */
	if (fdt_parse((const void *)dtb, &fdt) != 0) {
		for (;;)
			cpu_wfi();
	}
	poweroff_init(fdt.psci_hvc);

	/* Machine lines: CPSR, UART base, CNTFRQ. */
	uart_init(fdt.uart);
	uart_puts("armv7\n");
	asm volatile("mrs %0, cpsr" : "=r"(cpsr));
	uart_puts("cpsr ");
	uart_hex(cpsr);
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
	 * addresses from the tree. vm_on writes TTBR0 and sets SCTLR.M.
	 * shell_start brings up the disk and returns to user mode in sh.
	 * The printed timebase is CNTFRQ, which sleep uses as well.
	 */
	gic_init(fdt.gicd, fdt.gicc);
	timer_set_irq(fdt.timer_irq);
	vm_init(fdt.uart, fdt.virtio, fdt.nvirtio, fdt.gicd, fdt.gicc);
	vm_on();
	uart_puts("ttbr0 on\n");
	fdt.timebase = freq;
	shell_start(&fdt);
}
