#include "rv32.h"

/*
 * Supervisor timer interrupt: scause has the high bit set and the low
 * bits equal 5. The handler asks OpenSBI to move the next deadline to
 * the end of the time register, so this boot takes one tick.
 */

extern void trap_entry(void);

static volatile int timer_fired;

static uint64_t rdtime(void)
{
	uint32_t hi;
	uint32_t lo;
	uint32_t hi2;

	do {
		asm volatile("csrr %0, timeh" : "=r"(hi));
		asm volatile("csrr %0, time" : "=r"(lo));
		asm volatile("csrr %0, timeh" : "=r"(hi2));
	} while (hi != hi2);
	return ((uint64_t)hi << 32) | lo;
}

void trap_init(void)
{
	uint32_t zero = 0;
	uint32_t vector = (uint32_t)trap_entry;

	asm volatile("csrw sie, %0" :: "r"(zero));
	asm volatile("csrw stvec, %0" :: "r"(vector));
}

void trap_handler(void)
{
	uint32_t cause;
	uint32_t epc;

	asm volatile("csrr %0, scause" : "=r"(cause));
	asm volatile("csrr %0, sepc" : "=r"(epc));
	if (cause == 0x80000005u) {
		timer_fired = 1;
		sbi_set_timer(~(uint64_t)0);
		return;
	}
	uart_puts("trap scause ");
	uart_hex(cause);
	uart_puts(" sepc ");
	uart_hex(epc);
	uart_putc('\n');
	sbi_shutdown();
}

void timer_arm(uint32_t timebase)
{
	uint32_t stie = 1u << 5;
	uint32_t sie = 1u << 1;

	sbi_set_timer(rdtime() + (uint64_t)timebase / 100u);
	asm volatile("csrs sie, %0" :: "r"(stie));
	asm volatile("csrs sstatus, %0" :: "r"(sie));
}

void timer_wait(void)
{
	uint32_t sie = 1u << 1;

	while (!timer_fired)
		asm volatile("wfi");
	asm volatile("csrc sstatus, %0" :: "r"(sie));
	uart_puts("timer\n");
}
