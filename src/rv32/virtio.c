#include "rv32.h"

/*
 * virtio-mmio block device. The address comes from the device tree;
 * the device id in the MMIO header says which slot holds the disk.
 * One queue, one read at a time. The request, the data, and the
 * descriptor rings are ordinary kernel memory. The device uses those
 * physical addresses, which match the kernel's virtual addresses.
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
	wr(0x80, (uint32_t)desc);
	wr(0x84, 0);
	wr(0x90, (uint32_t)&avail);
	wr(0x94, 0);
	wr(0xa0, (uint32_t)&used);
	wr(0xa4, 0);
	asm volatile("fence w, w" ::: "memory");
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
	wr(0x40, (uint32_t)&desc >> 12);
	wr(0x70, ACK | DRIVER | DRIVER_OK);
	return 0;
}

int virtio_init(const uint32_t *bases, int n)
{
	int i;

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
		memset(desc, 0, sizeof desc);
		memset((void *)&avail, 0, sizeof avail);
		memset((void *)&used, 0, sizeof used);
		if (ver == 2)
			return modern_setup();
		if (ver == 1)
			return legacy_setup();
		return -1;
	}
	return -1;
}

uint32_t virtio_base(void)
{
	return (uint32_t)regs;
}

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
	desc[0].addr = (uint32_t)&req;
	desc[0].len = 16;
	desc[0].flags = DESC_NEXT;
	desc[0].next = 1;
	desc[1].addr = (uint32_t)sector;
	desc[1].len = 512;
	desc[1].flags = DESC_NEXT | DESC_WRITE;
	desc[1].next = 2;
	desc[2].addr = (uint32_t)&status;
	desc[2].len = 1;
	desc[2].flags = DESC_WRITE;
	desc[2].next = 0;
	idx = avail.idx;
	avail.ring[idx % 8] = 0;
	asm volatile("fence w, w" ::: "memory");
	avail.idx = (uint16_t)(idx + 1u);
	asm volatile("fence w, w" ::: "memory");
	wr(0x50, 0);
	want = (uint16_t)(idx + 1u);
	spins = 0;
	while (used.idx != want) {
		if (++spins > 10000000u)
			return -1;
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
