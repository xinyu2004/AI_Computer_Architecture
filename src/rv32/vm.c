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
 *
 * User programs live at 0x10000, with a stack page at 0x80000. Those
 * virtual pages point at kernel memory, so the kernel fills them by
 * physical address. The U bit lets the program read them after sret.
 */

#define PTE_V (1u << 0)
#define PTE_R (1u << 1)
#define PTE_W (1u << 2)
#define PTE_X (1u << 3)
#define PTE_U (1u << 4)
#define PTE_A (1u << 6)
#define PTE_D (1u << 7)

#define NLEAF 4
#define USER_BASE 0x10000u
#define USER_CODE_PAGES 16
#define USER_STACK_VA 0x80000u

extern char _start[];
extern char _end[];

static uint32_t root[1024] __attribute__((aligned(4096)));
static uint32_t leaf[NLEAF][1024] __attribute__((aligned(4096)));
static int nleaf;
static uint8_t user_pages[USER_CODE_PAGES][4096] __attribute__((aligned(4096)));
static uint8_t user_stack[4096] __attribute__((aligned(4096)));
static uint8_t saved_code[USER_CODE_PAGES][4096];
static uint8_t *active_stack;

	/*
	 * vpn1 is va[31:22]. A present root entry names a leaf table: the
	 * physical page number is shifted back out of bits [31:10]. A new
	 * table is recorded as PPN << 10 with the V bit.
	 */
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

	/* vpn0 is va[21:12]. The leaf stores the physical page the same way. */
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

	/*
	 * Identity map: virtual address equals physical address, so the
	 * pointers C already holds stay valid when satp is written. The
	 * kernel pages are executable. Device pages are not.
	 */
void vm_init(uint32_t uart, const uint32_t *virtio, int nvirtio, uint32_t test,
	    uint32_t plic)
{
	uint32_t flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
	uint32_t mmio = PTE_V | PTE_R | PTE_W | PTE_A | PTE_D;
	uint32_t plic_off[4];
	int i;

	plic_off[0] = 0;
	plic_off[1] = 0x2000u;
	plic_off[2] = 0x200000u;
	plic_off[3] = 0x201000u;
	if (map_range((uint32_t)_start, (uint32_t)_end, flags) != 0)
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
	uint32_t flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_U | PTE_A | PTE_D;
	uint32_t stack = PTE_V | PTE_R | PTE_W | PTE_U | PTE_A | PTE_D;
	int i;

	for (i = 0; i < USER_CODE_PAGES; i++) {
		uint32_t va = USER_BASE + (uint32_t)i * 4096u;
		uint32_t pa = (uint32_t)user_pages[i];

		if (map_page(va, pa, flags) != 0)
			poweroff();
	}
	active_stack = user_stack;
	if (map_page(USER_STACK_VA, (uint32_t)user_stack, stack) != 0)
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
	uint32_t stack = PTE_V | PTE_R | PTE_W | PTE_U | PTE_A | PTE_D;

	active_stack = page;
	if (map_page(USER_STACK_VA, (uint32_t)page, stack) != 0)
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
		return (uint32_t)user_pages[(va - USER_BASE) >> 12] + (va & 0xfffu);
	if (va >= USER_STACK_VA && va < USER_STACK_TOP && active_stack != 0)
		return (uint32_t)active_stack + (va & 0xfffu);
	return 0;
}

	/*
	 * satp bit 31 selects Sv32. The low bits are the root table's
	 * physical page number. sfence.vma drops translations cached
	 * from before the write.
	 */
void vm_on(void)
{
	uint32_t satp = (1u << 31) | ((uint32_t)root >> 12);

	asm volatile("sfence.vma" ::: "memory");
	asm volatile("csrw satp, %0" :: "r"(satp) : "memory");
	asm volatile("sfence.vma" ::: "memory");
}
