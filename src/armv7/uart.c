#include "armv7.h"

/*
 * PL011 UART. The base comes from the device tree. The flag register
 * is at offset 0x18. Bit 5 is the transmit FIFO full bit. Bit 4 is the
 * receive FIFO empty bit. The data register is at offset 0.
 */

#define UARTFR 0x18u
#define FR_RXFE (1u << 4)
#define FR_TXFF (1u << 5)
#define UARTLCR_H 0x2cu
#define UARTCR 0x30u
#define UARTIMSC 0x38u

static volatile uint32_t *uart;

static uint32_t rd(uint32_t off)
{
	return uart[off / 4u];
}

static void wr(uint32_t off, uint32_t val)
{
	uart[off / 4u] = val;
}

	/*
	 * UARTCR off, then the integer and fractional baud divisors.
	 * LCR_H: WLEN is bits [6:5] = 11 for 8 data bits, FEN is bit 4.
	 * IMSC stays clear, so the UART itself raises no interrupt.
	 * UARTCR then enables the UART, the transmitter, and the receiver.
	 */
void uart_init(uint32_t base)
{
	uart = (volatile uint32_t *)base;
	wr(UARTCR, 0);
	wr(0x24, 1);
	wr(0x28, 0);
	wr(UARTLCR_H, (3u << 5) | (1u << 4));
	wr(UARTIMSC, 0);
	wr(UARTCR, (1u << 0) | (1u << 8) | (1u << 9));
}

	/*
	 * Bit 4 of FR is RXFE. The prompt waits here with IRQs enabled, so
	 * a timer tick can still be taken. The receive FIFO is polled
	 * again after each wake.
	 */
int uart_getc(void)
{
	while ((rd(UARTFR) & FR_RXFE) != 0) {
		irq_on();
		cpu_wfi();
	}
	return (int)(rd(0) & 0xffu);
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
	while ((rd(UARTFR) & FR_TXFF) != 0)
		;
	wr(0, (uint8_t)c);
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
