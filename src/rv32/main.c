#include "rv32.h"

/*
 * A short RV32 kernel for the same five jobs as the 80386 path:
 * boot, page tables, a timer interrupt, a task switch, and one page copy.
 * _start begins in M-mode and mrets to S-mode. Boot prints the machine
 * lines, turns page tables on, and loads sh. The shell asks for the
 * timer, the task switch, and the page copy.
 */

static uint32_t page_src[1024] __attribute__((aligned(4096)));
static uint32_t page_dst[1024] __attribute__((aligned(4096)));
static uint8_t task1_stack[4096] __attribute__((aligned(16)));
static struct context ctx0;
static struct context ctx1;
static uint32_t boot_timebase;

static void copy_page(volatile uint32_t *dst, const volatile uint32_t *src)
{
	uint32_t i;

	for (i = 0; i < 1024; i++)
		dst[i] = src[i];
}

static void task1(void)
{
	uart_puts("task 1\n");
	swtch(&ctx1, &ctx0);
}

void run_timer(void)
{
	timer_arm(boot_timebase);
	timer_wait();
	uart_puts("timer\n");
}

void run_schedule(void)
{
	ctx1.ra = (uint32_t)task1;
	ctx1.sp = ((uint32_t)task1_stack + sizeof task1_stack) & ~15u;
	swtch(&ctx0, &ctx1);
	uart_puts("task 0\n");
}

void run_copy(void)
{
	uint32_t i;

	for (i = 0; i < 1024; i++)
		page_src[i] = 0xa5000000u + i;
	copy_page(page_dst, page_src);
	if (page_dst[0] != page_src[0] || page_dst[1023] != page_src[1023]) {
		uart_puts("copy_page mismatch\n");
		poweroff();
	}
	uart_puts("copy_page 4096\n");
}

void main(uint32_t hart, uint32_t dtb)
{
	struct fdt_info fdt;

	if (fdt_parse((const void *)dtb, &fdt) != 0) {
		for (;;)
			asm volatile("wfi");
	}
	poweroff_at(fdt.test);
	boot_timebase = fdt.timebase;

	uart_init(fdt.uart);
	uart_puts("rv32\n");
	uart_puts("hart ");
	uart_u32(hart);
	uart_putc('\n');
	uart_puts("uart ");
	uart_hex(fdt.uart);
	uart_putc('\n');
	uart_puts("timebase ");
	uart_u32(fdt.timebase);
	uart_putc('\n');

	trap_init();
	vm_init(fdt.uart, fdt.virtio, fdt.nvirtio, fdt.test);
	vm_on();
	uart_puts("sv32 on\n");
	shell_start(&fdt);
}
