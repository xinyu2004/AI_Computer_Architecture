#ifndef ARMV7_H
#define ARMV7_H

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;
typedef int int32_t;

/* Callee-saved bank. swtch writes these and stays in SVC. */
struct context {
	uint32_t r4;
	uint32_t r5;
	uint32_t r6;
	uint32_t r7;
	uint32_t r8;
	uint32_t r9;
	uint32_t r10;
	uint32_t r11;
	uint32_t lr;
	uint32_t sp;
};

/*
 * Matches the 68-byte SVC frame: r0-r12, user sp, user lr, the return
 * pc (LR_svc), and SPSR.
 */
struct trapframe {
	uint32_t r[13];
	uint32_t sp_usr;
	uint32_t lr_usr;
	uint32_t pc;
	uint32_t spsr;
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
	uint32_t gicd;
	uint32_t gicc;
	uint32_t timer_irq;
	int psci_hvc;
};

int fdt_parse(const void *dtb, struct fdt_info *out);

void poweroff_init(int use_hvc);
void poweroff(void);

void vm_init(uint32_t uart, const uint32_t *virtio, int nvirtio,
	    uint32_t gicd, uint32_t gicc);
void vm_on(void);
void vm_map_user(void);
void vm_user_clear(void);
void vm_save_code(void);
void vm_restore_code(void);
void vm_set_stack(void *page);
void vm_use_shell_stack(void);
uint32_t vm_user_pa(uint32_t va);

void swtch(struct context *old, struct context *next);

void gic_init(uint32_t dist, uint32_t cpu);
void gic_enable(uint32_t irq);
uint32_t gic_iar(void);
void gic_eoi(uint32_t irq);

int virtio_init(const struct fdt_info *fdt);
uint32_t virtio_base(void);
uint32_t virtio_irq(void);
int virtio_read(uint32_t sector, void *dst);
void virtio_ack(void);

int fs_init(void);
int fs_read(const char *name, void *dst, uint32_t cap, uint32_t *out_len);
int fs_list(char *dst, uint32_t cap);
int fs_exists(const char *name);

extern uint32_t name_block;

void trap_init(void);
void timer_set_irq(uint32_t irq);
void timer_arm(uint32_t timebase);
uint32_t timer_freq(void);
void sleep_ms(uint32_t ms);
void irq_handler(void);
void fault_handler(void);
void copy_page(uint32_t *dst, const uint32_t *src);

extern char current_name[28];
extern struct trapframe child_tf;

void user_enter(uint32_t entry, uint32_t user_sp);
void user_resume(void);
void syscall(struct trapframe *tf);
void shell_start(const struct fdt_info *fdt);

static inline void irq_off(void)
{
	asm volatile("cpsid i" ::: "memory");
}

/* cpsie i clears CPSR.I. The timer can then be taken. */
static inline void irq_on(void)
{
	asm volatile("cpsie i" ::: "memory");
}

static inline void cpu_wfi(void)
{
	asm volatile("wfi" ::: "memory");
}

static inline void cpu_dsb(void)
{
	asm volatile("dsb" ::: "memory");
}

#define USER_STACK_TOP 0x81000u

#endif
