#include "rv32.h"

/*
 * The launch script writes images/rv32.img. Sector 0 holds the magic
 * and the file count. Sector 1 is the directory. File bytes follow,
 * each file starting on a sector boundary. Magic bytes are V 2 S F.
 */

#define FS_MAGIC 0x46533256u

struct dent {
	char name[28];
	uint32_t off;
	uint32_t len;
};

/* Thirty-two sectors. A second read of one of them returns this copy. */
#define NCACHE 32

struct cbuf {
	uint32_t sector;
	int valid;
	uint8_t data[512];
};

static struct dent dents[8];
static int ndents;
static struct cbuf cache[NCACHE];
static int cache_next;

uint32_t name_block;

static int bread(uint32_t sector_no, void *dst);

static int streq(const char *a, const char *b)
{
	while (*a != '\0' && *a == *b) {
		a++;
		b++;
	}
	return *a == '\0' && *b == '\0';
}

int fs_init(void)
{
	uint8_t sec[512];
	uint32_t magic;
	uint32_t n;
	uint32_t i;

	if (bread(0, sec) != 0)
		return -1;
	magic = (uint32_t)sec[0] | ((uint32_t)sec[1] << 8) |
		((uint32_t)sec[2] << 16) | ((uint32_t)sec[3] << 24);
	n = (uint32_t)sec[4] | ((uint32_t)sec[5] << 8) |
	    ((uint32_t)sec[6] << 16) | ((uint32_t)sec[7] << 24);
	if (magic != FS_MAGIC || n == 0 || n > 8)
		return -1;
	if (bread(1, sec) != 0)
		return -1;
	ndents = (int)n;
	for (i = 0; i < n; i++)
		memcpy(&dents[i], sec + i * sizeof(struct dent), sizeof(struct dent));
	return 0;
}

__attribute__((noinline))
static int bread(uint32_t sector_no, void *dst)
{
	int i;
	struct cbuf *b;

	for (i = 0; i < NCACHE; i++) {
		if (cache[i].valid && cache[i].sector == sector_no) {
			memcpy(dst, cache[i].data, 512);
			return 0;
		}
	}
	b = &cache[cache_next];
	cache_next = (cache_next + 1) % NCACHE;
	if (virtio_read(sector_no, b->data) != 0)
		return -1;
	b->sector = sector_no;
	b->valid = 1;
	memcpy(dst, b->data, 512);
	return 0;
}

__attribute__((noinline))
static int lookup(const char *name)
{
	int i;

	for (i = 0; i < ndents; i++) {
		if (streq(dents[i].name, name)) {
			name_block = dents[i].off / 512u;
			return i;
		}
	}
	return -1;
}

int fs_read(const char *name, void *dst, uint32_t cap, uint32_t *out_len)
{
	int slot = lookup(name);
	uint32_t off;
	uint32_t len;
	uint32_t done;
	uint8_t sec[512];

	if (slot < 0)
		return -1;
	off = dents[slot].off;
	len = dents[slot].len;
	if (len > cap)
		return -1;
	done = 0;
	while (done < len) {
		uint32_t sector = (off + done) / 512u;
		uint32_t skip = (off + done) % 512u;
		uint32_t chunk = 512u - skip;

		if (chunk > len - done)
			chunk = len - done;
		if (bread(sector, sec) != 0)
			return -1;
		memcpy((uint8_t *)dst + done, sec + skip, chunk);
		done += chunk;
	}
	*out_len = len;
	return 0;
}

int fs_exists(const char *name)
{
	return lookup(name) >= 0;
}

int fs_list(char *dst, uint32_t cap)
{
	uint32_t used = 0;
	int i;

	for (i = 0; i < ndents; i++) {
		uint32_t k = 0;

		while (dents[i].name[k] != '\0' && k < 27u) {
			if (used + 2u >= cap)
				return -1;
			dst[used++] = dents[i].name[k++];
		}
		if (used + 1u > cap)
			return -1;
		dst[used++] = '\n';
	}
	return (int)used;
}
