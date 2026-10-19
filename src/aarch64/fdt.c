#include "aarch64.h"

/*
 * Walk the flattened device tree QEMU passed in x0. The UART, GIC, and
 * virtio reg properties are child addresses on the soc bus. On this
 * virt board that bus publishes an empty ranges property, so the child
 * address is the physical address.
 *
 * A GIC interrupt specifier is three cells: type, number, flags. Type 0
 * is an SPI and the GIC id is the number plus 32. Type 1 is a PPI and
 * the id is the number plus 16. The non-secure physical timer is PPI 14.
 */

#define FDT_MAGIC 0xd00dfeedu
#define FDT_BEGIN_NODE 1u
#define FDT_END_NODE 2u
#define FDT_PROP 3u
#define FDT_NOP 4u
#define FDT_END 9u

struct fdt_frame {
	uint32_t ac;
	uint32_t sc;
	uint32_t child_ac;
	uint32_t child_sc;
	int phys;
	int child_phys;
	int is_uart;
	int is_virtio;
	int is_gic;
	int is_psci;
	int is_timer;
	int have_reg;
	int have_reg2;
	int psci_hvc;
	uint32_t reg;
	uint32_t reg2;
	uint32_t irqs[4];
	int nirq;
};

static uint32_t fdt32(const uint8_t *p)
{
	return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
	       ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

static uint32_t align4(uint32_t x)
{
	return (x + 3u) & ~3u;
}

static int name_is(const char *s, const char *lit)
{
	while (*lit) {
		if (*s++ != *lit++)
			return 0;
	}
	return *s == '\0';
}

static int contains_string(const char *s, uint32_t len, const char *needle)
{
	uint32_t nlen = 0;
	uint32_t i = 0;

	while (needle[nlen] != '\0')
		nlen++;
	while (i < len) {
		uint32_t start = i;
		uint32_t k;

		while (i < len && s[i] != '\0')
			i++;
		if (i - start == nlen) {
			for (k = 0; k < nlen; k++) {
				if (s[start + k] != needle[k])
					break;
			}
			if (k == nlen)
				return 1;
		}
		if (i < len && s[i] == '\0')
			i++;
	}
	return 0;
}

static int read_reg(const uint8_t *val, uint32_t len, uint32_t ac, uint32_t *out)
{
	uint32_t hi;
	uint32_t lo;

	if (ac < 1 || ac > 2 || len < ac * 4u)
		return -1;
	if (ac == 1) {
		*out = fdt32(val);
		return 0;
	}
	hi = fdt32(val);
	lo = fdt32(val + 4);
	if (hi != 0)
		return -1;
	*out = lo;
	return 0;
}

static void read_irqs(struct fdt_frame *fr, const uint8_t *val, uint32_t len)
{
	uint32_t n;
	uint32_t i;

	if (len < 12u || (len % 12u) != 0)
		return;
	n = len / 12u;
	for (i = 0; i < n && fr->nirq < 4; i++) {
		uint32_t type = fdt32(val + i * 12u);
		uint32_t id = fdt32(val + i * 12u + 4u);

		if (type == 0)
			fr->irqs[fr->nirq++] = id + 32u;
		else if (type == 1)
			fr->irqs[fr->nirq++] = id + 16u;
	}
}

int fdt_parse(const void *dtb, struct fdt_info *out)
{
	const uint8_t *b = dtb;
	uint32_t magic;
	uint32_t totalsize;
	uint32_t off_struct;
	uint32_t off_strings;
	uint32_t p;
	int depth = -1;
	int got_uart = 0;
	struct fdt_frame stack[12];

	out->uart = 0;
	out->timebase = 0;
	out->nvirtio = 0;
	out->gicd = 0;
	out->gicc = 0;
	out->timer_irq = 30;
	out->psci_hvc = 1;

	if (dtb == 0)
		return -1;
	magic = fdt32(b);
	totalsize = fdt32(b + 4);
	off_struct = fdt32(b + 8);
	off_strings = fdt32(b + 12);
	if (magic != FDT_MAGIC || off_struct >= totalsize || off_strings >= totalsize)
		return -1;

	p = off_struct;
	while (p + 4u <= totalsize) {
		uint32_t token = fdt32(b + p);
		const char *prop;
		const uint8_t *val;
		uint32_t len;
		uint32_t nameoff;
		struct fdt_frame *fr;

		p += 4;
		if (token == FDT_BEGIN_NODE) {
			uint32_t start = p;

			if (depth + 1 >= (int)(sizeof stack / sizeof stack[0]))
				return -1;
			depth++;
			fr = &stack[depth];
			if (depth == 0) {
				fr->ac = 2;
				fr->sc = 1;
				fr->phys = 1;
				fr->child_ac = 2;
				fr->child_sc = 1;
				fr->child_phys = 1;
			} else {
				fr->ac = stack[depth - 1].child_ac;
				fr->sc = stack[depth - 1].child_sc;
				fr->phys = stack[depth - 1].child_phys;
				fr->child_ac = fr->ac;
				fr->child_sc = fr->sc;
				fr->child_phys = fr->phys;
			}
			fr->is_uart = 0;
			fr->is_virtio = 0;
			fr->is_gic = 0;
			fr->is_psci = 0;
			fr->is_timer = 0;
			fr->have_reg = 0;
			fr->have_reg2 = 0;
			fr->psci_hvc = 1;
			fr->reg = 0;
			fr->reg2 = 0;
			fr->nirq = 0;
			while (p < totalsize && b[p] != '\0')
				p++;
			if (p >= totalsize)
				return -1;
			p = align4(start + (p - start) + 1u);
			continue;
		}
		if (token == FDT_END_NODE) {
			if (depth < 0)
				return -1;
			fr = &stack[depth];
			if (fr->is_uart && !got_uart) {
				if (!fr->phys || !fr->have_reg)
					return -1;
				out->uart = fr->reg;
				got_uart = 1;
			}
			if (fr->is_virtio && fr->phys && fr->have_reg &&
			    out->nvirtio < FDT_VIRTIO_MAX) {
				out->virq[out->nvirtio] = fr->nirq > 0 ? fr->irqs[0] : 0;
				out->virtio[out->nvirtio++] = fr->reg;
			}
			if (fr->is_gic && fr->phys && fr->have_reg && out->gicd == 0) {
				out->gicd = fr->reg;
				if (fr->have_reg2)
					out->gicc = fr->reg2;
			}
			if (fr->is_psci)
				out->psci_hvc = fr->psci_hvc;
			if (fr->is_timer) {
				int i;

				for (i = 0; i < fr->nirq; i++) {
					if (fr->irqs[i] == 30u)
						out->timer_irq = 30u;
				}
			}
			depth--;
			continue;
		}
		if (token == FDT_NOP)
			continue;
		if (token == FDT_END)
			break;
		if (token != FDT_PROP || depth < 0 || p + 8u > totalsize)
			return -1;

		len = fdt32(b + p);
		nameoff = fdt32(b + p + 4);
		p += 8;
		if (p + len > totalsize || off_strings + nameoff >= totalsize)
			return -1;
		prop = (const char *)(b + off_strings + nameoff);
		val = b + p;
		fr = &stack[depth];
		if (name_is(prop, "#address-cells") && len == 4)
			fr->child_ac = fdt32(val);
		else if (name_is(prop, "#size-cells") && len == 4)
			fr->child_sc = fdt32(val);
		else if (name_is(prop, "ranges"))
			fr->child_phys = (len == 0);
		else if (name_is(prop, "compatible") &&
			 contains_string((const char *)val, len, "arm,pl011"))
			fr->is_uart = 1;
		else if (name_is(prop, "compatible") &&
			 contains_string((const char *)val, len, "virtio,mmio"))
			fr->is_virtio = 1;
		else if (name_is(prop, "compatible") &&
			 (contains_string((const char *)val, len, "arm,cortex-a15-gic") ||
			  contains_string((const char *)val, len, "arm,gic-400")))
			fr->is_gic = 1;
		else if (name_is(prop, "compatible") &&
			 contains_string((const char *)val, len, "arm,psci"))
			fr->is_psci = 1;
		else if (name_is(prop, "compatible") &&
			 (contains_string((const char *)val, len, "arm,armv7-timer") ||
			  contains_string((const char *)val, len, "arm,armv8-timer")))
			fr->is_timer = 1;
		else if (name_is(prop, "interrupts"))
			read_irqs(fr, val, len);
		else if (name_is(prop, "method") && len >= 3) {
			fr->psci_hvc = (val[0] == 'h' && val[1] == 'v' && val[2] == 'c');
		} else if (name_is(prop, "reg")) {
			uint32_t addr;
			uint32_t stride = (fr->ac + fr->sc) * 4u;

			if (read_reg(val, len, fr->ac, &addr) == 0) {
				fr->reg = addr;
				fr->have_reg = 1;
			}
			if (stride >= 4u && len >= stride * 2u &&
			    read_reg(val + stride, len - stride, fr->ac, &addr) == 0) {
				fr->reg2 = addr;
				fr->have_reg2 = 1;
			}
		} else if (name_is(prop, "clock-frequency") && len == 4 &&
			   out->timebase == 0) {
			out->timebase = fdt32(val);
		}
		p = align4(p + len);
	}
	if (!got_uart || out->gicd == 0 || out->gicc == 0)
		return -1;
	return 0;
}
