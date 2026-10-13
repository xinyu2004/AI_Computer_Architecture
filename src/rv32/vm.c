#include "rv32.h"

/*
 * Sv32 is a two-level table of 4KiB pages. satp bit 31 selects that mode,
 * and the low bits are the physical page number of the root table.
 * A leaf page-table entry holds the physical page and the permission bits.
 * There is no GDT and no TSS. The kernel is mapped at its own physical
 * address, so pointers stay valid after paging is turned on.
 *
 * V is present. R W X are read, write, and execute. A and D are the
 * accessed and dirty bits; this kernel sets both so the first reference
 * does not fault. The UART page is not executable.
 */

#define PTE_V (1u << 0)
#define PTE_R (1u << 1)
#define PTE_W (1u << 2)
#define PTE_X (1u << 3)
#define PTE_A (1u << 6)
#define PTE_D (1u << 7)

#define NLEAF 4

extern char _start[];
extern char _end[];

static uint32_t root[1024] __attribute__((aligned(4096)));
static uint32_t leaf[NLEAF][1024] __attribute__((aligned(4096)));
static int nleaf;

static uint32_t *leaf_for(uint32_t vpn1)
{
	uint32_t *table;
	uint32_t pte;

	pte = root[vpn1];
	if (pte & PTE_V)
		return (uint32_t *)((pte >> 10) << 12);
	if (nleaf >= NLEAF)
		return 0;
	table = leaf[nleaf++];
	root[vpn1] = ((uint32_t)table >> 12) << 10 | PTE_V;
	return table;
}

static int map_page(uint32_t va, uint32_t pa, uint32_t flags)
{
	uint32_t vpn1 = va >> 22;
	uint32_t vpn0 = (va >> 12) & 0x3ffu;
	uint32_t *table = leaf_for(vpn1);

	if (table == 0)
		return -1;
	table[vpn0] = ((pa >> 12) << 10) | flags;
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

void vm_init(uint32_t uart)
{
	uint32_t flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
	uint32_t mmio = PTE_V | PTE_R | PTE_W | PTE_A | PTE_D;

	if (map_range((uint32_t)_start, (uint32_t)_end, flags) != 0)
		sbi_shutdown();
	if (map_page(uart & ~0xfffu, uart & ~0xfffu, mmio) != 0)
		sbi_shutdown();
}

void vm_on(void)
{
	uint32_t satp = (1u << 31) | ((uint32_t)root >> 12);

	asm volatile("sfence.vma" ::: "memory");
	asm volatile("csrw satp, %0" :: "r"(satp) : "memory");
	asm volatile("sfence.vma" ::: "memory");
}
