#include "armv7.h"

/*
 * GICv2. The distributor and the CPU interface come from the device
 * tree. A peripheral interrupt id is the SPI number plus 32. The CPU
 * interface acknowledge register is the id of the interrupt that fired.
 */

static volatile uint32_t *dist;
static volatile uint32_t *cpu;

void gic_init(uint32_t dist_base, uint32_t cpu_base)
{
	dist = (volatile uint32_t *)dist_base;
	cpu = (volatile uint32_t *)cpu_base;
	/*
	 * This board enters in Secure SVC. gic_enable marks every line
	 * Group 1. Both EnableGrp1 bits must be set or the CPU interface
	 * never forwards the line. AckCtl must be set too: a Secure read
	 * of the acknowledge register otherwise returns 1022 and leaves
	 * the Group 1 interrupt pending.
	 */
	dist[0] = 3;
	cpu[0] = 7;
	cpu[1] = 0xff;
}

	/*
	 * IGROUPR at 0x80: a set bit marks the line Group 1. IPRIORITYR at
	 * 0x400 stores one byte per interrupt; 0x80 is a mid priority.
	 * ITARGETSR at 0x800 names CPU 0, and only an SPI (id 32 and up)
	 * has a target byte. ISENABLER at 0x100 is what lets the line
	 * through.
	 */
void gic_enable(uint32_t irq)
{
	volatile uint8_t *base = (volatile uint8_t *)dist;

	dist[(0x80u + (irq / 32u) * 4u) / 4u] |= 1u << (irq % 32u);
	base[0x400u + irq] = 0x80;
	if (irq >= 32u)
		base[0x800u + irq] = 0x01;
	dist[(0x100u + (irq / 32u) * 4u) / 4u] = 1u << (irq % 32u);
}

	/* GICC_IAR at 0x0c. The low 10 bits are the interrupt id. */
uint32_t gic_iar(void)
{
	return cpu[0x0cu / 4u];
}

	/* GICC_EOIR at 0x10. The id written here is the one IAR returned. */
void gic_eoi(uint32_t irq)
{
	cpu[0x10u / 4u] = irq;
}
