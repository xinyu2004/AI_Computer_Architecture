#include "aarch64.h"

/*
 * The generic timer compares CNTPCT_EL0 with CNTP_CVAL_EL0. The
 * interrupt id comes from the device tree and is read back from the
 * GIC CPU interface. The interval is the timer frequency divided by 100.
 */

static uint32_t timer_interval;
static uint32_t ticks_per_ms;
static uint32_t timer_irq;

	/* CNTPCT_EL0 is the 64-bit physical count. */
static uint64_t cntpct(void)
{
	uint64_t now;

	asm volatile("mrs %0, cntpct_el0" : "=r"(now));
	return now;
}

static void set_cval(uint64_t when)
{
	asm volatile("msr cntp_cval_el0, %0" :: "r"(when));
}

static void timer_rearm(void)
{
	set_cval(cntpct() + timer_interval);
}

uint32_t timer_freq(void)
{
	uint64_t freq;

	asm volatile("mrs %0, cntfrq_el0" : "=r"(freq));
	return (uint32_t)freq;
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
	uint64_t esr;
	uint64_t far;

	asm volatile("mrs %0, esr_el1" : "=r"(esr));
	asm volatile("mrs %0, far_el1" : "=r"(far));
	uart_puts("abort esr ");
	uart_hex((uint32_t)esr);
	uart_puts(" far ");
	uart_hex((uint32_t)far);
	uart_putc('\n');
	poweroff();
}

void timer_set_irq(uint32_t irq)
{
	timer_irq = irq;
}

	/*
	 * CNTP_CVAL_EL0 is the compare value. CNTP_CTL_EL0 bit 0 enables
	 * the timer. gic_enable unmasks that line, and daifclr #2 lets
	 * the CPU take it while the shell waits for a key.
	 */
void timer_arm(uint32_t timebase)
{
	uint64_t ctl = 1;

	timer_interval = timebase / 100u;
	ticks_per_ms = timebase / 1000u;
	if (timer_interval == 0)
		timer_interval = 1;
	if (ticks_per_ms == 0)
		ticks_per_ms = 1;
	timer_rearm();
	asm volatile("msr cntp_ctl_el0, %0" :: "r"(ctl));
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
