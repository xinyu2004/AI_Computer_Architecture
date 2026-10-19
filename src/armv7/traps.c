#include "armv7.h"

/*
 * The generic timer compares CNTPCT with CNTP_CVAL. The interrupt id
 * comes from the device tree and is read back from the GIC CPU
 * interface. The interval is the timer frequency divided by 100.
 */

static uint32_t timer_interval;
static uint32_t ticks_per_ms;
static uint32_t timer_irq;

	/* CNTPCT is 64 bits. mrrc returns the low half and the high half. */
static uint64_t cntpct(void)
{
	uint32_t lo;
	uint32_t hi;

	asm volatile("mrrc p15, 0, %0, %1, c14" : "=r"(lo), "=r"(hi));
	return ((uint64_t)hi << 32) | lo;
}

static void set_cval(uint64_t when)
{
	uint32_t lo = (uint32_t)when;
	uint32_t hi = (uint32_t)(when >> 32);

	asm volatile("mcrr p15, 2, %0, %1, c14" :: "r"(lo), "r"(hi));
}

static void timer_rearm(void)
{
	set_cval(cntpct() + timer_interval);
}

uint32_t timer_freq(void)
{
	uint32_t freq;

	asm volatile("mrc p15, 0, %0, c14, c0, 0" : "=r"(freq));
	return freq;
}

void trap_init(void)
{
}

	/*
	 * GICC_IAR is the id. 1022 and 1023 are spurious and must not be
	 * EOI'd. 30 is the non-secure physical timer on this board, and
	 * the virtio line is the SPI number plus 32. Every accepted id
	 * is written back to GICC_EOIR.
	 */
void irq_handler(void)
{
	uint32_t id = gic_iar();

	if (id >= 1020u)
		return;
	if (id == timer_irq)
		timer_rearm();
	else if (id == virtio_irq())
		virtio_ack();
	gic_eoi(id);
}

void fault_handler(void)
{
	uint32_t fsr;
	uint32_t far;

	asm volatile("mrc p15, 0, %0, c5, c0, 0" : "=r"(fsr));
	asm volatile("mrc p15, 0, %0, c6, c0, 0" : "=r"(far));
	uart_puts("abort fsr ");
	uart_hex(fsr);
	uart_puts(" far ");
	uart_hex(far);
	uart_putc('\n');
	poweroff();
}

void timer_set_irq(uint32_t irq)
{
	timer_irq = irq;
}

	/*
	 * CNTP_CVAL is the compare value. CNTP_CTL bit 0 enables the
	 * timer. gic_enable unmasks that line, and cpsie i lets the CPU
	 * take it while the shell waits for a key.
	 */
void timer_arm(uint32_t timebase)
{
	uint32_t ctl = 1;

	timer_interval = timebase / 100u;
	ticks_per_ms = timebase / 1000u;
	if (timer_interval == 0)
		timer_interval = 1;
	if (ticks_per_ms == 0)
		ticks_per_ms = 1;
	timer_rearm();
	asm volatile("mcr p15, 0, %0, c14, c2, 1" :: "r"(ctl));
	gic_enable(timer_irq);
	irq_on();
}

void sleep_ms(uint32_t ms)
{
	uint64_t deadline = cntpct() + (uint64_t)ms * ticks_per_ms;

	while (cntpct() < deadline) {
		irq_on();
		cpu_wfi();
	}
}
