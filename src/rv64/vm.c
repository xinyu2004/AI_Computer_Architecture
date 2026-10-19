#include "rv64.h"

/*
 * Sv39 is a three-level table of 4KiB pages. satp bits 63:60 are the mode,
 * 8 names Sv39, and the low bits are the physical page number of the root.
 * A leaf page-table entry holds the physical page and the permission bits.
 * A leaf page-table entry holds the physical page and the permission bits.
 * There is no GDT and no TSS. The kernel is mapped at its own physical
 * address, so pointers stay valid after paging is turned on.
 *
 * V is present. R W X are read, write, and execute. A and D are the
 * accessed and dirty bits; this kernel sets both so the first reference
 * does not fault. The UART page is not executable.
 *
 * User programs live at 0x10000, with a stack page at 0x80000. Those
 * virtual pages point at kernel memory, so the kernel fills them by
 * physical address. The U bit lets the program read them after sret.
 */

#define PTE_V (1ull << 0)
#define PTE_R (1ull << 1)
#define PTE_W (1ull << 2)
#define PTE_X (1ull << 3)
#define PTE_U (1ull << 4)
#define PTE_A (1ull << 6)
#define PTE_D (1ull << 7)

#define NPAGES 12
#define USER_BASE 0x10000u
#define USER_CODE_PAGES 16
#define USER_STACK_VA 0x80000u

extern char _start[];
extern char _end[];

static uint64_t root[512] __attribute__((aligned(4096)));
static uint64_t pgtbl[NPAGES][512] __attribute__((aligned(4096)));
static int npages;
static uint8_t user_pages[USER_CODE_PAGES][4096] __attribute__((aligned(4096)));
static uint8_t user_stack[4096] __attribute__((aligned(4096)));
static uint8_t saved_code[USER_CODE_PAGES][4096];
static uint8_t *active_stack;

static uint64_t pte_pa(uint64_t pa)
{
	return (pa >> 12) << 10;
}

static uint64_t *next_table(uint64_t *table, uint32_t idx)
{
	uint64_t pte = table[idx];
	uint64_t *child;

	if (pte & PTE_V)
		return (uint64_t *)(uintptr_t)((pte >> 10) << 12);
	if (npages >= NPAGES)
		return 0;
	child = pgtbl[npages++];
	table[idx] = pte_pa((uint64_t)(uintptr_t)child) | PTE_V;
	return child;
}

static int map_page(uint32_t va, uint32_t pa, uint64_t flags)
{
	uint64_t *mid = next_table(root, (va >> 30) & 0x1ffu);
	uint64_t *leaf;

	if (mid == 0)
		return -1;
	leaf = next_table(mid, (va >> 21) & 0x1ffu);
	if (leaf == 0)
		return -1;
	leaf[(va >> 12) & 0x1ffu] = pte_pa(pa) | flags;
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

void vm_init(uint32_t uart, const uint32_t *virtio, int nvirtio, uint32_t test,
	    uint32_t plic)
{
	uint64_t flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
	uint64_t mmio = PTE_V | PTE_R | PTE_W | PTE_A | PTE_D;
	uint32_t plic_off[4];
	int i;

	plic_off[0] = 0;
	plic_off[1] = 0x2000u;
	plic_off[2] = 0x200000u;
	plic_off[3] = 0x201000u;
	if (map_range((uint32_t)(uintptr_t)_start, (uint32_t)(uintptr_t)_end, flags) != 0)
		poweroff();
	if (map_page(uart & ~0xfffu, uart & ~0xfffu, mmio) != 0)
		poweroff();
	for (i = 0; i < nvirtio; i++) {
		if (map_page(virtio[i] & ~0xfffu, virtio[i] & ~0xfffu, mmio) != 0)
			poweroff();
	}
	if (test != 0 && map_page(test & ~0xfffu, test & ~0xfffu, mmio) != 0)
		poweroff();
	for (i = 0; i < 4; i++) {
		uint32_t pa = (plic + plic_off[i]) & ~0xfffu;

		if (plic != 0 && map_page(pa, pa, mmio) != 0)
			poweroff();
	}
}

void vm_map_user(void)
{
	uint64_t flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_U | PTE_A | PTE_D;
	uint64_t stack = PTE_V | PTE_R | PTE_W | PTE_U | PTE_A | PTE_D;
	int i;

	for (i = 0; i < USER_CODE_PAGES; i++) {
		uint32_t va = USER_BASE + (uint32_t)i * 4096u;
		uint32_t pa = (uint32_t)(uintptr_t)user_pages[i];

		if (map_page(va, pa, flags) != 0)
			poweroff();
	}
	active_stack = user_stack;
	if (map_page(USER_STACK_VA, (uint32_t)(uintptr_t)user_stack, stack) != 0)
		poweroff();
	asm volatile("sfence.vma" ::: "memory");
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
	uint64_t stack = PTE_V | PTE_R | PTE_W | PTE_U | PTE_A | PTE_D;

	active_stack = page;
	if (map_page(USER_STACK_VA, (uint32_t)(uintptr_t)page, stack) != 0)
		poweroff();
	asm volatile("sfence.vma" ::: "memory");
}

void vm_use_shell_stack(void)
{
	vm_set_stack(user_stack);
}

uint32_t vm_user_pa(uint32_t va)
{
	if (va >= USER_BASE && va < USER_BASE + USER_CODE_PAGES * 4096u)
		return (uint32_t)(uintptr_t)user_pages[(va - USER_BASE) >> 12] + (va & 0xfffu);
	if (va >= USER_STACK_VA && va < USER_STACK_TOP && active_stack != 0)
		return (uint32_t)(uintptr_t)active_stack + (va & 0xfffu);
	return 0;
}

void vm_on(void)
{
	uint64_t satp = (8ull << 60) | ((uint64_t)(uintptr_t)root >> 12);

	asm volatile("sfence.vma" ::: "memory");
	asm volatile("csrw satp, %0" :: "r"(satp) : "memory");
	asm volatile("sfence.vma" ::: "memory");
}
