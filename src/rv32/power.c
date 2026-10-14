#include "rv32.h"

/*
 * The board's power-off device. compatible is sifive,test0 or
 * sifive,test1. A store of 0x5555 asks that device to power off.
 * The address comes from the device tree.
 */

static volatile uint32_t *power;

void poweroff_at(uint32_t addr)
{
	if (addr != 0)
		power = (volatile uint32_t *)addr;
}

void poweroff(void)
{
	if (power != 0)
		*power = 0x5555;
	for (;;)
		asm volatile("wfi");
}
