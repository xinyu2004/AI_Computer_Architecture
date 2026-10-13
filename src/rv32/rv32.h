#ifndef RV32_H
#define RV32_H

typedef unsigned char uint8_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;
typedef int int32_t;

struct sbiret {
	long error;
	long value;
};

struct context {
	uint32_t ra;
	uint32_t sp;
	uint32_t s0;
	uint32_t s1;
	uint32_t s2;
	uint32_t s3;
	uint32_t s4;
	uint32_t s5;
	uint32_t s6;
	uint32_t s7;
	uint32_t s8;
	uint32_t s9;
	uint32_t s10;
	uint32_t s11;
};

void uart_init(uint32_t base);
void uart_putc(char c);
void uart_puts(const char *s);
void uart_hex(uint32_t value);
void uart_u32(uint32_t value);

int fdt_uart_timebase(const void *dtb, uint32_t *uart, uint32_t *timebase);

struct sbiret sbi_call(int eid, int fid, uint32_t arg0, uint32_t arg1);
void sbi_set_timer(uint64_t when);
void sbi_shutdown(void);

void vm_init(uint32_t uart);
void vm_on(void);

void swtch(struct context *old, struct context *next);

void trap_init(void);
void timer_arm(uint32_t timebase);
void timer_wait(void);

#endif
