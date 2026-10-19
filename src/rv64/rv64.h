#ifndef RV32_H
#define RV32_H

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;
typedef int int32_t;
typedef uint64_t reg_t;
typedef unsigned long uintptr_t;

/* ra, sp, and s0-s11, eight bytes each. swtch writes this block. */
struct context {
	reg_t ra;
	reg_t sp;
	reg_t s0;
	reg_t s1;
	reg_t s2;
	reg_t s3;
	reg_t s4;
	reg_t s5;
	reg_t s6;
	reg_t s7;
	reg_t s8;
	reg_t s9;
	reg_t s10;
	reg_t s11;
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
void copy_page(uint64_t *dst, const uint64_t *src);

extern char current_name[28];

/*
 * 280 bytes, eight bytes per slot. x[0] is unused. sepc is at 256,
 * sstatus at 264, and kernel_sp at 272.
 */
struct trapframe {
	reg_t x[32];
	reg_t sepc;
	reg_t sstatus;
	reg_t kernel_sp;
};

void user_enter(reg_t entry, reg_t user_sp);
void user_resume(void);
void syscall(struct trapframe *tf);
void shell_start(const struct fdt_info *fdt);

#define USER_STACK_TOP 0x81000u

#endif
