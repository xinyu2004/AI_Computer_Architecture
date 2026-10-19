#include "aarch64.h"

/*
 * PSCI SYSTEM_OFF. The conduit is the method string in the device tree,
 * hvc on this virt board. The function id is 0x84000008.
 */

static int use_hvc = 1;

void poweroff_init(int hvc)
{
	use_hvc = hvc;
}

void poweroff(void)
{
	register uint64_t x0 asm("x0") = 0x84000008ull;

	if (use_hvc)
		asm volatile("hvc #0" :: "r"(x0));
	else
		asm volatile("smc #0" :: "r"(x0));
	for (;;)
		cpu_wfi();
}
