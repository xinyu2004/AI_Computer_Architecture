#include "rv32.h"

/*
 * A short RV32 kernel for the same five jobs as the 80386 path:
 * boot, page tables, a timer interrupt, a task switch, and one page copy.
 * OpenSBI owns M-mode. This code stays in S-mode.
 */

static uint32_t page_src[1024] __attribute__((aligned(4096)));
static uint32_t page_dst[1024] __attribute__((aligned(4096)));
static uint8_t task1_stack[4096] __attribute__((aligned(16)));
static struct context ctx0;
static struct context ctx1;

static void copy_page(volatile uint32_t *dst, const volatile uint32_t *src)
{
	uint32_t i;

	for (i = 0; i < 1024; i++)
		dst[i] = src[i];
}

static void task1(void)
{
	uint32_t i;

	uart_puts("task 1\n");
	for (i = 0; i < 1024; i++)
		page_src[i] = 0xa5000000u + i;
	copy_page(page_dst, page_src);
	if (page_dst[0] != page_src[0] || page_dst[1023] != page_src[1023]) {
		uart_puts("copy_page mismatch\n");
		sbi_shutdown();
	}
	uart_puts("copy_page 4096\n");
	swtch(&ctx1, &ctx0);
}

void main(uint32_t hart, uint32_t dtb)
{
	uint32_t uart = 0;
	uint32_t timebase = 0;

	if (fdt_uart_timebase((const void *)dtb, &uart, &timebase) != 0)
		sbi_shutdown();

	uart_init(uart);
	uart_puts("rv32\n");
	uart_puts("hart ");
	uart_u32(hart);
	uart_putc('\n');
	uart_puts("uart ");
	uart_hex(uart);
	uart_putc('\n');
	uart_puts("timebase ");
	uart_u32(timebase);
	uart_putc('\n');

	trap_init();
	vm_init(uart);
	vm_on();
	uart_puts("sv32 on\n");

	timer_arm(timebase);
	timer_wait();

	ctx1.ra = (uint32_t)task1;
	ctx1.sp = ((uint32_t)task1_stack + sizeof task1_stack) & ~15u;
	swtch(&ctx0, &ctx1);
	uart_puts("task 0\n");
	sbi_shutdown();
}
