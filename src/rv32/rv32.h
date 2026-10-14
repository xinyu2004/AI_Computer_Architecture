#ifndef RV32_H
#define RV32_H

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;
typedef int int32_t;

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

void *memset(void *dst, int c, unsigned n);
void *memcpy(void *dst, const void *src, unsigned n);

void uart_init(uint32_t base);
void uart_putc(char c);
void uart_puts(const char *s);
void uart_hex(uint32_t value);
void uart_u32(uint32_t value);
int uart_getc(void);

#define FDT_VIRTIO_MAX 8

struct fdt_info {
	uint32_t uart;
	uint32_t timebase;
	uint32_t virtio[FDT_VIRTIO_MAX];
	int nvirtio;
	uint32_t test;
};

int fdt_parse(const void *dtb, struct fdt_info *out);

void poweroff_at(uint32_t addr);
void poweroff(void);

void vm_init(uint32_t uart, const uint32_t *virtio, int nvirtio,
	    uint32_t test);
void vm_on(void);
void vm_map_user(void);
void vm_user_clear(void);
uint32_t vm_user_pa(uint32_t va);

void swtch(struct context *old, struct context *next);

int virtio_init(const uint32_t *bases, int n);
uint32_t virtio_base(void);
int virtio_read(uint32_t sector, void *dst);

int fs_init(void);
int fs_read(const char *name, void *dst, uint32_t cap, uint32_t *out_len);
int fs_list(char *dst, uint32_t cap);

void trap_init(void);
void timer_arm(uint32_t timebase);
void timer_wait(void);
void run_timer(void);
void run_schedule(void);
void run_copy(void);

struct trapframe {
	uint32_t x[32];
	uint32_t sepc;
	uint32_t sstatus;
	uint32_t kernel_sp;
};

void user_enter(uint32_t entry, uint32_t user_sp);
void syscall(struct trapframe *tf);
void shell_start(const struct fdt_info *fdt);

#define USER_STACK_TOP 0x81000u

#endif
