#include "rv32.h"

/*
 * A 16550 UART found in the device tree. Register 0 is the transmitter.
 * Register 5 is the line status; bit 5 says the transmitter can take a byte.
 * Register 3 is the line control. 0x03 selects 8 data bits, no parity,
 * one stop bit. These registers are memory, not an I/O port.
 */
#define LSR 5
#define LSR_DR 0x01
#define LSR_THRE 0x20

static volatile uint8_t *uart;

void uart_init(uint32_t base)
{
	uart = (volatile uint8_t *)base;
	uart[3] = 0x03;
	uart[1] = 0x00;
}

int uart_getc(void)
{
	uint32_t sie = 1u << 1;

	/* The prompt blocks here. Leave SIE on and wake from the timer
	 * so a tick still arrives while nobody is typing. The 16550
	 * receive bit is not an interrupt in this kernel; the next tick
	 * polls the line status again.
	 */
	while ((uart[LSR] & LSR_DR) == 0) {
		asm volatile("csrs sstatus, %0" :: "r"(sie));
		asm volatile("wfi");
	}
	return uart[0];
}

int read_line(char *buf, uint32_t cap)
{
	uint32_t n = 0;

	if (cap == 0)
		return 0;
	for (;;) {
		int c = uart_getc();

		if (c == '\r')
			c = '\n';
		if (c == 0x7f || c == 0x08) {
			if (n > 0) {
				n--;
				uart_puts("\b \b");
			}
			continue;
		}
		if (c == '\n') {
			uart_putc('\n');
			break;
		}
		if (n + 1u < cap) {
			buf[n++] = (char)c;
			uart_putc((char)c);
		}
	}
	return (int)n;
}

void uart_putc(char c)
{
	if (c == '\n')
		uart_putc('\r');
	while ((uart[LSR] & LSR_THRE) == 0)
		;
	uart[0] = (uint8_t)c;
}

void uart_puts(const char *s)
{
	while (*s)
		uart_putc(*s++);
}

void uart_hex(uint32_t value)
{
	int i;

	uart_puts("0x");
	for (i = 7; i >= 0; i--) {
		uint32_t nibble = (value >> (i * 4)) & 0xfu;
		uart_putc((char)(nibble < 10 ? '0' + nibble : 'a' + nibble - 10));
	}
}

void uart_u32(uint32_t value)
{
	char buf[10];
	int i = 10;

	if (value == 0) {
		uart_putc('0');
		return;
	}
	while (value > 0 && i > 0) {
		buf[--i] = (char)('0' + value % 10);
		value /= 10;
	}
	while (i < 10)
		uart_putc(buf[i++]);
}
