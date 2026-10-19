#include "armv7.h"

/*
 * Short-descriptor pages. TTBCR.EAE stays 0, so TTBR0 is a first-level
 * table of 1MiB sections, and each present entry names a second-level
 * table of 4KiB small pages. The kernel is mapped at its own physical
 * address, so pointers stay valid after the MMU is turned on.
 *
 * Domain 0 is a client, so the AP bits are checked. Kernel pages are
 * privileged read/write. User code and the user stack set the user
 * access bits. The UART page is not executable.
 *
 * User programs live at 0x10000, with a stack page at 0x80000. Those
 * virtual pages point at kernel memory, so the kernel fills them by
 * physical address.
 */

#define L2_POOL 16
#define USER_BASE 0x10000u
#define USER_CODE_PAGES 16
#define USER_STACK_VA 0x80000u

#define SMALL_PAGE (1u << 1)
#define AP_PRIV (1u << 4)
#define AP_USER (3u << 4)
#define TEX_NORMAL (1u << 6)
#define XN (1u << 0)

#define KPAGE (SMALL_PAGE | AP_PRIV | TEX_NORMAL)
#define UPAGE (SMALL_PAGE | AP_USER | TEX_NORMAL)
#define USTACK (UPAGE | XN)
#define MMIO (SMALL_PAGE | AP_PRIV | XN)

extern char _start[];
extern char _end[];

static uint32_t l1[4096] __attribute__((aligned(16384)));
static uint32_t l2[L2_POOL][256] __attribute__((aligned(1024)));
static int nl2;
static uint8_t user_pages[USER_CODE_PAGES][4096] __attribute__((aligned(4096)));
static uint8_t user_stack[4096] __attribute__((aligned(4096)));
static uint8_t saved_code[USER_CODE_PAGES][4096];
static uint8_t *active_stack;

static uint32_t *l2_for(uint32_t index)
{
	uint32_t desc = l1[index];
	uint32_t *table;

	if ((desc & 3u) == 1u)
		return (uint32_t *)(desc & 0xfffffc00u);
	if (nl2 >= L2_POOL)
		return 0;
	table = l2[nl2++];
	l1[index] = ((uint32_t)table & 0xfffffc00u) | 1u;
	return table;
}

static int map_page(uint32_t va, uint32_t pa, uint32_t flags)
{
	uint32_t *table = l2_for(va >> 20);

	if (table == 0)
		return -1;
	table[(va >> 12) & 0xffu] = (pa & 0xfffff000u) | flags;
	return 0;
}

static int map_range(uint32_t start, uint32_t end, uint32_t flags)
{
	uint32_t pa;

	start &= ~0xfffu;
	end = (end + 0xfffu) & ~0xfffu;
	for (pa = start; pa < end; pa += 4096) {
		if (map_page(pa, pa, flags) != 0)
			return -1;
	}
	return 0;
}

static void vm_flush(void)
{
	uint32_t zero = 0;

	asm volatile("mcr p15, 0, %0, c8, c7, 0" :: "r"(zero) : "memory");
	cpu_dsb();
	asm volatile("isb" ::: "memory");
}

void vm_init(uint32_t uart, const uint32_t *virtio, int nvirtio,
	    uint32_t gicd, uint32_t gicc)
{
	int i;

	if (map_range((uint32_t)_start, (uint32_t)_end, KPAGE) != 0)
		poweroff();
	if (map_page(uart & ~0xfffu, uart & ~0xfffu, MMIO) != 0)
		poweroff();
	for (i = 0; i < nvirtio; i++) {
		uint32_t pa = virtio[i] & ~0xfffu;

		if (map_page(pa, pa, MMIO) != 0)
			poweroff();
	}
	if (gicd != 0 && map_page(gicd & ~0xfffu, gicd & ~0xfffu, MMIO) != 0)
		poweroff();
	if (gicc != 0 && map_page(gicc & ~0xfffu, gicc & ~0xfffu, MMIO) != 0)
		poweroff();
}

void vm_map_user(void)
{
	int i;

	for (i = 0; i < USER_CODE_PAGES; i++) {
		uint32_t va = USER_BASE + (uint32_t)i * 4096u;
		uint32_t pa = (uint32_t)user_pages[i];

		if (map_page(va, pa, UPAGE) != 0)
			poweroff();
	}
	active_stack = user_stack;
	if (map_page(USER_STACK_VA, (uint32_t)user_stack, USTACK) != 0)
		poweroff();
	vm_flush();
}

void vm_user_clear(void)
{
	memset(user_pages, 0, sizeof user_pages);
}

void vm_save_code(void)
{
	memcpy(saved_code, user_pages, sizeof user_pages);
}

void vm_restore_code(void)
{
	memcpy(user_pages, saved_code, sizeof user_pages);
}

void vm_set_stack(void *page)
{
	active_stack = page;
	if (map_page(USER_STACK_VA, (uint32_t)page, USTACK) != 0)
		poweroff();
	vm_flush();
}

void vm_use_shell_stack(void)
{
	vm_set_stack(user_stack);
}

uint32_t vm_user_pa(uint32_t va)
{
	if (va >= USER_BASE && va < USER_BASE + USER_CODE_PAGES * 4096u)
		return (uint32_t)user_pages[(va - USER_BASE) >> 12] + (va & 0xfffu);
	if (va >= USER_STACK_VA && va < USER_STACK_TOP && active_stack != 0)
		return (uint32_t)active_stack + (va & 0xfffu);
	return 0;
}

void vm_on(void)
{
	uint32_t ttbr = (uint32_t)l1;
	uint32_t ttbcr;
	uint32_t sctlr;
	uint32_t dacr = 1;
	uint32_t zero = 0;

	asm volatile("mcr p15, 0, %0, c3, c0, 0" :: "r"(dacr));
	asm volatile("mrc p15, 0, %0, c2, c0, 2" : "=r"(ttbcr));
	ttbcr &= ~(1u << 31);
	ttbcr &= ~7u;
	ttbcr |= 1u << 5;
	asm volatile("mcr p15, 0, %0, c2, c0, 2" :: "r"(ttbcr));
	asm volatile("mcr p15, 0, %0, c2, c0, 0" :: "r"(ttbr) : "memory");
	asm volatile("mcr p15, 0, %0, c8, c7, 0" :: "r"(zero) : "memory");
	cpu_dsb();
	asm volatile("isb" ::: "memory");
	asm volatile("mrc p15, 0, %0, c1, c0, 0" : "=r"(sctlr));
	sctlr &= ~((1u << 1) | (1u << 2) | (1u << 12) | (1u << 28) | (1u << 29));
	sctlr |= 1u << 0;
	asm volatile("mcr p15, 0, %0, c1, c0, 0" :: "r"(sctlr) : "memory");
	cpu_dsb();
	asm volatile("isb" ::: "memory");
}
