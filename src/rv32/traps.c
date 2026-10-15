#include "rv32.h"

/*
 * Supervisor timer interrupt: scause has the high bit set and the low
 * bits equal 5. stimecmp is the Sstc compare register. The handler
 * moves the next deadline one interval ahead. The interval is
 * timebase/100, 10ms when the timebase is 10MHz. The prompt leaves
 * the interrupt enabled, so ticks continue while the shell waits.
 */

extern void trap_entry(void);

static uint32_t timer_interval;
static uint32_t ticks_per_ms;

static void set_timer(uint64_t when)
{
	uint32_t lo = (uint32_t)when;
	uint32_t hi = (uint32_t)(when >> 32);

	asm volatile("csrw 0x14d, %0" :: "r"(lo));
	asm volatile("csrw 0x15d, %0" :: "r"(hi));
}

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
	asm volatile("csrw sscratch, %0" :: "r"(zero));
	asm volatile("csrw stvec, %0" :: "r"(vector));
}

void trap_handler(struct trapframe *tf)
{
	uint32_t cause;

	asm volatile("csrr %0, scause" : "=r"(cause));
	if (cause == 0x80000005u) {
		set_timer(rdtime() + timer_interval);
		return;
	}
	if (cause == 8u) {
		syscall(tf);
		return;
	}
	uart_puts("trap scause ");
	uart_hex(cause);
	uart_puts(" sepc ");
	uart_hex(tf->sepc);
	uart_putc('\n');
	poweroff();
}

void timer_arm(uint32_t timebase)
{
	uint32_t stie = 1u << 5;
	uint32_t sie = 1u << 1;

	timer_interval = timebase / 100u;
	ticks_per_ms = timebase / 1000u;
	if (timer_interval == 0)
		timer_interval = 1;
	if (ticks_per_ms == 0)
		ticks_per_ms = 1;
	set_timer(rdtime() + timer_interval);
	asm volatile("csrs sie, %0" :: "r"(stie));
	asm volatile("csrs sstatus, %0" :: "r"(sie));
}

void sleep_ms(uint32_t ms)
{
	uint64_t deadline = rdtime() + (uint64_t)ms * ticks_per_ms;
	uint32_t sie = 1u << 1;

	while (rdtime() < deadline) {
		asm volatile("csrs sstatus, %0" :: "r"(sie));
		asm volatile("wfi");
	}
}
