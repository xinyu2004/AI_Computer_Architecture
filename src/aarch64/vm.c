#include "aarch64.h"

/*
 * Four-kilobyte pages in TTBR0_EL1. T0SZ is 25, so a virtual address is
 * 39 bits and the walk starts at level 1. A level-3 descriptor is a
 * page. The kernel is mapped at its own physical address, so pointers
 * stay valid after the table is turned on. Caches stay off.
 *
 * User programs live at 0x10000, with a stack page at 0x80000. Those
 * virtual pages point at kernel memory, so the kernel fills them by
 * physical address.
 */

#define PTE_PAGE 0x3ull
#define PTE_AF (1ull << 10)
#define PTE_AP_USER (1ull << 6)
#define PTE_PXN (1ull << 53)
#define PTE_UXN (1ull << 54)
#define PTE_NORMAL (1ull << 2)
#define PTE_DEVICE 0ull

#define NPAGES 16
#define USER_BASE 0x10000ull
#define USER_CODE_PAGES 16
#define USER_STACK_VA 0x80000ull

extern char _start[];
extern char _end[];

static uint64_t root[512] __attribute__((aligned(4096)));
static uint64_t pgtbl[NPAGES][512] __attribute__((aligned(4096)));
static int npages;
static uint8_t user_pages[USER_CODE_PAGES][4096] __attribute__((aligned(4096)));
static uint8_t user_stack[4096] __attribute__((aligned(4096)));
static uint8_t saved_code[USER_CODE_PAGES][4096];
static uint8_t *active_stack;

static uint64_t *next_table(uint64_t *table, uint32_t idx)
{
	uint64_t pte = table[idx];
	uint64_t *child;

	if ((pte & 0x3ull) == PTE_PAGE)
		return (uint64_t *)(uintptr_t)(pte & ~0xfffull);
	if (npages >= NPAGES)
		return 0;
	child = pgtbl[npages++];
	table[idx] = ((uint64_t)(uintptr_t)child & ~0xfffull) | PTE_PAGE;
	return child;
}

static int map_page(uint64_t va, uint64_t pa, uint64_t flags)
{
	uint64_t *l2 = next_table(root, (uint32_t)((va >> 30) & 0x1ffu));
	uint64_t *l3;

	if (l2 == 0)
		return -1;
	l3 = next_table(l2, (uint32_t)((va >> 21) & 0x1ffu));
	if (l3 == 0)
		return -1;
	l3[(va >> 12) & 0x1ffu] = (pa & ~0xfffull) | flags | PTE_PAGE | PTE_AF;
	return 0;
}

static int map_range(uint64_t start, uint64_t end, uint64_t flags)
{
	uint64_t pa;

	start &= ~0xfffull;
	end = (end + 0xfffull) & ~0xfffull;
	for (pa = start; pa < end; pa += 4096) {
		if (map_page(pa, pa, flags) != 0)
			return -1;
	}
	return 0;
}

static void vm_flush(void)
{
	asm volatile("tlbi vmalle1" ::: "memory");
	asm volatile("dsb sy" ::: "memory");
	asm volatile("isb" ::: "memory");
}

void vm_init(uint32_t uart, const uint32_t *virtio, int nvirtio,
	    uint32_t gicd, uint32_t gicc)
{
	uint64_t kern = PTE_NORMAL | PTE_UXN;
	uint64_t mmio = PTE_DEVICE | PTE_UXN | PTE_PXN;
	int i;

	if (map_range((uint64_t)(uintptr_t)_start, (uint64_t)(uintptr_t)_end, kern) != 0)
		poweroff();
	if (map_page(uart & ~0xfffu, uart & ~0xfffu, mmio) != 0)
		poweroff();
	for (i = 0; i < nvirtio; i++) {
		if (map_page(virtio[i] & ~0xfffu, virtio[i] & ~0xfffu, mmio) != 0)
			poweroff();
	}
	if (map_page(gicd & ~0xfffu, gicd & ~0xfffu, mmio) != 0)
		poweroff();
	if (map_page(gicc & ~0xfffu, gicc & ~0xfffu, mmio) != 0)
		poweroff();
}

void vm_on(void)
{
	uint64_t mair = 0x44ull << 8;
	uint64_t tcr = 25ull | (1ull << 23) | (2ull << 32);
	uint64_t ttbr = (uint64_t)(uintptr_t)root;
	uint64_t sctlr;

	asm volatile("msr mair_el1, %0" :: "r"(mair));
	asm volatile("msr tcr_el1, %0" :: "r"(tcr));
	asm volatile("msr ttbr0_el1, %0" :: "r"(ttbr));
	vm_flush();
	asm volatile("mrs %0, sctlr_el1" : "=r"(sctlr));
	sctlr &= ~((1ull << 1) | (1ull << 2) | (1ull << 3) | (1ull << 12) | (1ull << 19));
	sctlr |= 1ull;
	asm volatile("msr sctlr_el1, %0" :: "r"(sctlr));
	asm volatile("isb" ::: "memory");
}

void vm_map_user(void)
{
	uint64_t code = PTE_NORMAL | PTE_AP_USER | PTE_PXN;
	uint64_t stack = PTE_NORMAL | PTE_AP_USER | PTE_UXN | PTE_PXN;
	int i;

	for (i = 0; i < USER_CODE_PAGES; i++) {
		uint64_t va = USER_BASE + (uint64_t)i * 4096u;
		uint64_t pa = (uint64_t)(uintptr_t)user_pages[i];

		if (map_page(va, pa, code) != 0)
			poweroff();
	}
	active_stack = user_stack;
	if (map_page(USER_STACK_VA, (uint64_t)(uintptr_t)user_stack, stack) != 0)
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
	uint64_t stack = PTE_NORMAL | PTE_AP_USER | PTE_UXN | PTE_PXN;

	active_stack = page;
	if (map_page(USER_STACK_VA, (uint64_t)(uintptr_t)page, stack) != 0)
		poweroff();
	vm_flush();
}

void vm_use_shell_stack(void)
{
	vm_set_stack(user_stack);
}

uint64_t vm_user_pa(uint64_t va)
{
	if (va >= USER_BASE && va < USER_BASE + USER_CODE_PAGES * 4096u)
		return (uint64_t)(uintptr_t)user_pages[(va - USER_BASE) >> 12] + (va & 0xfffu);
	if (va >= USER_STACK_VA && va < USER_STACK_TOP && active_stack != 0)
		return (uint64_t)(uintptr_t)active_stack + (va & 0xfffu);
	return 0;
}
