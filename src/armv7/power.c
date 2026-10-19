#include "armv7.h"

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
	register uint32_t r0 asm("r0") = 0x84000008u;

	if (use_hvc)
		asm volatile(".arch_extension virt\n\thvc #0" :: "r"(r0));
	else
		asm volatile(".arch_extension sec\n\tsmc #0" :: "r"(r0));
	for (;;)
		cpu_wfi();
}
