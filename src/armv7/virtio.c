#include "armv7.h"

/*
 * virtio-mmio block device. The address comes from the device tree.
 * One queue, one read at a time. A finished read raises the virtio
 * interrupt. virtio_read waits until the used ring advances, then the
 * IRQ handler acknowledges it through the GIC.
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

/*
 * Legacy virtio puts the descriptor table, the available ring, and the
 * used ring in one physically contiguous block. The page number of that
 * block is the queue address. A modern device takes the three addresses
 * separately; they still point into this block.
 */
struct vq {
	struct vdesc desc[8];
	volatile uint16_t aflags;
	volatile uint16_t aidx;
	uint16_t aring[8];
	volatile uint16_t uflags;
	volatile uint16_t uidx;
	struct {
		volatile uint32_t id;
		volatile uint32_t len;
	} uring[8];
};

static volatile uint32_t *regs;
static uint32_t disk_irq;
static struct vq queue __attribute__((aligned(4096)));
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
	wr(0x80, (uint32_t)queue.desc);
	wr(0x84, 0);
	wr(0x90, (uint32_t)&queue.aflags);
	wr(0x94, 0);
	wr(0xa0, (uint32_t)&queue.uflags);
	wr(0xa4, 0);
	cpu_dsb();
	wr(0x44, 1);
	wr(0x70, ACK | DRIVER | FEATURES_OK | DRIVER_OK);
	return 0;
}

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
	wr(0x40, (uint32_t)&queue >> 12);
	wr(0x70, ACK | DRIVER | DRIVER_OK);
	return 0;
}

int virtio_init(const struct fdt_info *fdt)
{
	int i;
	int n = fdt->nvirtio;
	const uint32_t *bases = fdt->virtio;

	disk_irq = 0;
	for (i = 0; i < n; i++) {
		uint32_t ver;
		uint32_t id;

		regs = (volatile uint32_t *)bases[i];
		if (rr(0x00) != MAGIC)
			continue;
		id = rr(0x08);
		if (id != DEV_BLK)
			continue;
		ver = rr(0x04);
		wr(0x70, 0);
		wr(0x70, ACK);
		wr(0x70, ACK | DRIVER);
		memset(&queue, 0, sizeof queue);
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
		if (disk_irq != 0)
			gic_enable(disk_irq);
		return 0;
	}
	return -1;
}

void virtio_ack(void)
{
	uint32_t intr;

	if (regs == 0)
		return;
	intr = rr(0x60);
	if (intr != 0)
		wr(0x64, intr);
}

uint32_t virtio_base(void)
{
	return (uint32_t)regs;
}

uint32_t virtio_irq(void)
{
	return disk_irq;
}

int virtio_read(uint32_t sector_no, void *dst)
{
	uint16_t idx;
	uint16_t want;
	uint32_t spins;

	req.type = 0;
	req.reserved = 0;
	req.sector = sector_no;
	status = 0xff;
	queue.desc[0].addr = (uint32_t)&req;
	queue.desc[0].len = 16;
	queue.desc[0].flags = DESC_NEXT;
	queue.desc[0].next = 1;
	queue.desc[1].addr = (uint32_t)sector;
	queue.desc[1].len = 512;
	queue.desc[1].flags = DESC_NEXT | DESC_WRITE;
	queue.desc[1].next = 2;
	queue.desc[2].addr = (uint32_t)&status;
	queue.desc[2].len = 1;
	queue.desc[2].flags = DESC_WRITE;
	queue.desc[2].next = 0;
	idx = queue.aidx;
	queue.aring[idx % 8] = 0;
	cpu_dsb();
	queue.aidx = (uint16_t)(idx + 1u);
	cpu_dsb();
	want = (uint16_t)(idx + 1u);
	if (disk_irq != 0)
		irq_off();
	wr(0x50, 0);
	spins = 0;
	while (queue.uidx != want) {
		if (++spins > 1000000u) {
			if (disk_irq != 0)
				irq_on();
			return -1;
		}
	}
	/*
	 * The used ring moves inside the notify write. The interrupt is
	 * already pending. Take it now, while nothing else is waiting on wfi,
	 * so the handler can claim it.
	 */
	if (disk_irq != 0)
		irq_on();
	cpu_dsb();
	virtio_ack();
	if (status != 0)
		return -1;
	memcpy(dst, sector, 512);
	return 0;
}
