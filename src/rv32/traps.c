#include "rv32.h"

/*
 * Supervisor timer interrupt: scause has the high bit set and the low
 * bits equal 5. stimecmp is the Sstc compare register. The handler
 * moves the next deadline to the end of the time register, so each
 * timer command takes one tick.
 */

extern void trap_entry(void);

static volatile int timer_fired;

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
		timer_fired = 1;
		set_timer(~(uint64_t)0);
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

	timer_fired = 0;
	set_timer(rdtime() + (uint64_t)timebase / 100u);
	asm volatile("csrs sie, %0" :: "r"(stie));
	asm volatile("csrs sstatus, %0" :: "r"(sie));
}

void timer_wait(void)
{
	uint32_t sie = 1u << 1;

	while (!timer_fired)
		asm volatile("wfi");
	asm volatile("csrc sstatus, %0" :: "r"(sie));
}
