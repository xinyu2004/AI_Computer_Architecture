#ifndef RV32_H
#define RV32_H

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;
typedef int int32_t;

/* ra, sp, and s0-s11. swtch writes this block; sret is separate. */
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
int read_line(char *buf, uint32_t cap);

#define FDT_VIRTIO_MAX 8

struct fdt_info {
	uint32_t uart;
	uint32_t timebase;
	uint32_t virtio[FDT_VIRTIO_MAX];
	uint32_t virq[FDT_VIRTIO_MAX];
	int nvirtio;
	uint32_t plic;
	int plic_context;
	uint32_t test;
};

int fdt_parse(const void *dtb, struct fdt_info *out);

void poweroff_at(uint32_t addr);
void poweroff(void);

void vm_init(uint32_t uart, const uint32_t *virtio, int nvirtio,
	    uint32_t test, uint32_t plic);
void vm_on(void);
void vm_map_user(void);
void vm_user_clear(void);
void vm_save_code(void);
void vm_restore_code(void);
void vm_set_stack(void *page);
void vm_use_shell_stack(void);
uint32_t vm_user_pa(uint32_t va);

void swtch(struct context *old, struct context *next);

int virtio_init(const struct fdt_info *fdt);
uint32_t virtio_base(void);
int virtio_read(uint32_t sector, void *dst);
void virtio_isr(void);

int fs_init(void);
int fs_read(const char *name, void *dst, uint32_t cap, uint32_t *out_len);
int fs_list(char *dst, uint32_t cap);
int fs_exists(const char *name);

extern uint32_t name_block;

void trap_init(void);
void timer_arm(uint32_t timebase);
void sleep_ms(uint32_t ms);
void copy_page(uint32_t *dst, const uint32_t *src);

extern char current_name[28];

/*
 * 144 bytes. x[0] is unused. sepc and sstatus follow the registers,
 * then the kernel stack used when the trap came from U-mode.
 */
struct trapframe {
	uint32_t x[32];
	uint32_t sepc;
	uint32_t sstatus;
	uint32_t kernel_sp;
};

void user_enter(uint32_t entry, uint32_t user_sp);
void user_resume(void);
void syscall(struct trapframe *tf);
void shell_start(const struct fdt_info *fdt);

#define USER_STACK_TOP 0x81000u

#endif
