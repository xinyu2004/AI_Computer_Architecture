#include "rv32.h"

/*
 * ecall from S-mode enters OpenSBI in M-mode. a7 is the extension id,
 * a6 the function id. a0 comes back as the error code.
 * TIME is "TIME" (0x54494D45), function 0, the absolute time of the
 * next interrupt. On RV32 that time is split across a0 and a1.
 * SRST is "SRST" (0x53525354), function 0, type 0, which shuts down.
 */
struct sbiret sbi_call(int eid, int fid, uint32_t arg0, uint32_t arg1)
{
	register uint32_t a0 asm("a0") = arg0;
	register uint32_t a1 asm("a1") = arg1;
	register uint32_t a6 asm("a6") = (uint32_t)fid;
	register uint32_t a7 asm("a7") = (uint32_t)eid;

	asm volatile("ecall"
		     : "+r"(a0), "+r"(a1)
		     : "r"(a6), "r"(a7)
		     : "memory");
	return (struct sbiret){ (long)a0, (long)a1 };
}

void sbi_set_timer(uint64_t when)
{
	struct sbiret ret;

	ret = sbi_call(0x54494D45, 0, (uint32_t)when, (uint32_t)(when >> 32));
	if (ret.error != 0) {
		uart_puts("sbi timer ");
		uart_hex((uint32_t)ret.error);
		uart_putc('\n');
		sbi_shutdown();
	}
}

void sbi_shutdown(void)
{
	sbi_call(0x53525354, 0, 0, 0);
	for (;;)
		asm volatile("wfi");
}
