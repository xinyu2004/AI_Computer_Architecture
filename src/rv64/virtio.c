#include "rv64.h"

/*
 * virtio-mmio block device. The address comes from the device tree;
 * the device id in the MMIO header says which slot holds the disk.
 * One queue, one read at a time. The request, the data, and the
 * descriptor rings are ordinary kernel memory. The device uses those
 * physical addresses, which match the kernel's virtual addresses.
 * A finished read raises the virtio-mmio interrupt. virtio_read waits
 * until the used ring advances, then virtio_isr claims it on the PLIC.
 */

#define MAGIC 0x74726976u
#define DEV_BLK 2u
#define ACK 1u
#define DRIVER 2u
#define DRIVER_OK 4u
#define FEATURES_OK 8u
#define DESC_NEXT 1u
#define DESC_WRITE 2u

struct vdesc {
	uint64_t addr;
	uint32_t len;
	uint16_t flags;
	uint16_t next;
};

struct vavail {
	volatile uint16_t flags;
	volatile uint16_t idx;
	uint16_t ring[8];
};

struct vused {
	volatile uint16_t flags;
	volatile uint16_t idx;
	struct {
		volatile uint32_t id;
		volatile uint32_t len;
	} ring[8];
};

struct breq {
	uint32_t type;
	uint32_t reserved;
	uint64_t sector;
};

static volatile uint32_t *regs;
static uint32_t plic_base;
static int plic_ctx;
static uint32_t disk_irq;
static struct vdesc desc[8] __attribute__((aligned(16)));
static struct vavail avail __attribute__((aligned(2)));
static struct vused used __attribute__((aligned(4)));
static struct breq req __attribute__((aligned(8)));
static uint8_t sector[512] __attribute__((aligned(4)));
static uint8_t status;

static uint32_t rr(uint32_t off)
{
	return regs[off / 4u];
}

static void wr(uint32_t off, uint32_t val)
{
	regs[off / 4u] = val;
}

	/*
	 * Version 2. Negotiate an empty feature set, then point the device
	 * at the descriptor table, the available ring, and the used ring.
	 * The addresses are 64 bits, written as a low word and a high word.
	 * Queue ready is register 0x44. DRIVER_OK lets the device run.
	 */
static int modern_setup(void)
{
	uint32_t high;
	uint32_t maxq;

	wr(0x14, 1);
	high = rr(0x10);
	if ((high & 1u) == 0)
		return -1;
	wr(0x24, 0);
	wr(0x20, 0);
	wr(0x24, 1);
	wr(0x20, 1);
	wr(0x70, ACK | DRIVER | FEATURES_OK);
	if ((rr(0x70) & FEATURES_OK) == 0)
		return -1;
	wr(0x30, 0);
	maxq = rr(0x34);
	if (maxq < 8)
		return -1;
	wr(0x38, 8);
	wr(0x80, (uint32_t)(uintptr_t)desc);
	wr(0x84, 0);
	wr(0x90, (uint32_t)(uintptr_t)&avail);
	wr(0x94, 0);
	wr(0xa0, (uint32_t)(uintptr_t)&used);
	wr(0xa4, 0);
	asm volatile("fence w, w" ::: "memory");
	wr(0x44, 1);
	wr(0x70, ACK | DRIVER | FEATURES_OK | DRIVER_OK);
	return 0;
}

	/*
	 * Version 1. The guest page size is 4096. The queue address
	 * register takes the page number of the descriptor table. The
	 * available and used rings follow it in the legacy layout.
	 */
static int legacy_setup(void)
{
	uint32_t maxq;

	wr(0x28, 4096);
	wr(0x20, 0);
	wr(0x30, 0);
	maxq = rr(0x34);
	if (maxq < 8)
		return -1;
	wr(0x38, 8);
	wr(0x3c, 4);
	wr(0x40, (uint32_t)((uintptr_t)&desc >> 12));
	wr(0x70, ACK | DRIVER | DRIVER_OK);
	return 0;
}

static volatile uint32_t *plic_word(uint32_t off)
{
	return (volatile uint32_t *)(uintptr_t)(plic_base + off);
}

	/*
	 * Priority 1, then the enable bit for this context, then a
	 * threshold of 0 so every priority is delivered. The claim
	 * register is read later in virtio_isr.
	 */
static void plic_enable(uint32_t irq)
{
	volatile uint32_t *prio = plic_word(irq * 4u);
	volatile uint32_t *en = plic_word(0x2000u + (uint32_t)plic_ctx * 0x80u +
					  (irq / 32u) * 4u);
	volatile uint32_t *th = plic_word(0x200000u + (uint32_t)plic_ctx * 0x1000u);

	*prio = 1;
	*en |= 1u << (irq % 32u);
	*th = 0;
}

int virtio_init(const struct fdt_info *fdt)
{
	int i;
	int n = fdt->nvirtio;
	const uint32_t *bases = fdt->virtio;

	plic_base = fdt->plic;
	plic_ctx = fdt->plic_context;
	disk_irq = 0;
	for (i = 0; i < n; i++) {
		uint32_t ver;
		uint32_t id;

		regs = (volatile uint32_t *)(uintptr_t)bases[i];
		if (rr(0x00) != MAGIC)
			continue;
		id = rr(0x08);
		if (id != DEV_BLK)
			continue;
		ver = rr(0x04);
		wr(0x70, 0);
		wr(0x70, ACK);
		wr(0x70, ACK | DRIVER);
		memset(desc, 0, sizeof desc);
		memset((void *)&avail, 0, sizeof avail);
		memset((void *)&used, 0, sizeof used);
		if (ver == 2) {
			if (modern_setup() != 0)
				return -1;
		} else if (ver == 1) {
			if (legacy_setup() != 0)
				return -1;
		} else {
			return -1;
		}
		disk_irq = fdt->virq[i];
		if (plic_base != 0 && disk_irq != 0) {
			uint32_t seie = 1u << 9;

			plic_enable(disk_irq);
			asm volatile("csrs sie, %0" :: "r"(seie));
		}
		return 0;
	}
	return -1;
}

void virtio_isr(void)
{
	volatile uint32_t *claim;
	uint32_t irq;
	uint32_t intr;

	if (plic_base == 0)
		return;
	claim = plic_word(0x200004u + (uint32_t)plic_ctx * 0x1000u);
	irq = *claim;
	if (irq == disk_irq && regs != 0) {
		intr = rr(0x60);
		if (intr != 0)
			wr(0x64, intr);
	}
	if (irq != 0)
		*claim = irq;
}

uint32_t virtio_base(void)
{
	return (uint32_t)(uintptr_t)regs;
}

	/*
	 * Three descriptors: the 16-byte request, a 512-byte buffer the
	 * device writes, and a one-byte status the device writes. QueueNotify
	 * is register 0x50. SIE is cleared around the wait so a timer tick
	 * does not run while the used index is being watched. On this board
	 * the device finishes the request inside that notify write, and wfi
	 * returns when the external interrupt is pending.
	 */
int virtio_read(uint32_t sector_no, void *dst)
{
	uint16_t idx;
	uint16_t want;
	uint32_t spins;
	uint32_t intr;

	req.type = 0;
	req.reserved = 0;
	req.sector = sector_no;
	status = 0xff;
	desc[0].addr = (uint64_t)(uintptr_t)&req;
	desc[0].len = 16;
	desc[0].flags = DESC_NEXT;
	desc[0].next = 1;
	desc[1].addr = (uint64_t)(uintptr_t)sector;
	desc[1].len = 512;
	desc[1].flags = DESC_NEXT | DESC_WRITE;
	desc[1].next = 2;
	desc[2].addr = (uint64_t)(uintptr_t)&status;
	desc[2].len = 1;
	desc[2].flags = DESC_WRITE;
	desc[2].next = 0;
	idx = avail.idx;
	avail.ring[idx % 8] = 0;
	asm volatile("fence w, w" ::: "memory");
	avail.idx = (uint16_t)(idx + 1u);
	asm volatile("fence w, w" ::: "memory");
	want = (uint16_t)(idx + 1u);
	if (plic_base != 0 && disk_irq != 0) {
		uint32_t sie = 1u << 1;

		asm volatile("csrc sstatus, %0" :: "r"(sie));
	}
	wr(0x50, 0);
	spins = 0;
	while (used.idx != want) {
		if (plic_base != 0 && disk_irq != 0)
			asm volatile("wfi");
		if (++spins > 1000000u) {
			if (plic_base != 0 && disk_irq != 0) {
				uint32_t sie = 1u << 1;

				asm volatile("csrs sstatus, %0" :: "r"(sie));
			}
			return -1;
		}
	}
	if (plic_base != 0 && disk_irq != 0) {
		uint32_t sie = 1u << 1;

		asm volatile("csrs sstatus, %0" :: "r"(sie));
	}
	asm volatile("fence r, r" ::: "memory");
	intr = rr(0x60);
	if (intr != 0)
		wr(0x64, intr);
	if (status != 0)
		return -1;
	memcpy(dst, sector, 512);
	return 0;
}
