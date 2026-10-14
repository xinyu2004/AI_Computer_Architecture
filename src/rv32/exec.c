#include "rv32.h"

/*
 * One user process. sh is loaded from the disk and entered in U-mode.
 * exec replaces that image. exit from any program other than sh loads
 * sh again. exit from sh shuts the machine down.
 *
 * The console is the UART. read waits for a line and echoes it.
 * There is no separate tty driver.
 */

#define SYS_EXIT 0u
#define SYS_WRITE 1u
#define SYS_READ 2u
#define SYS_EXEC 3u
#define SYS_LIST 4u
#define SYS_RUN 5u

#define JOB_TIMER 1u
#define JOB_SCHEDULE 2u
#define JOB_COPY 3u

struct elf32_ehdr {
	uint8_t ident[16];
	uint16_t type;
	uint16_t machine;
	uint32_t version;
	uint32_t entry;
	uint32_t phoff;
	uint32_t shoff;
	uint32_t flags;
	uint16_t ehsize;
	uint16_t phentsize;
	uint16_t phnum;
	uint16_t shentsize;
	uint16_t shnum;
	uint16_t shstrndx;
};

struct elf32_phdr {
	uint32_t type;
	uint32_t offset;
	uint32_t vaddr;
	uint32_t paddr;
	uint32_t filesz;
	uint32_t memsz;
	uint32_t flags;
	uint32_t align;
};

static int is_shell;
static uint8_t filebuf[16 * 1024];

static int copy_out(uint32_t va, const void *src, uint32_t len)
{
	uint32_t i;
	const uint8_t *s = src;

	for (i = 0; i < len; i++) {
		uint32_t pa = vm_user_pa(va + i);

		if (pa == 0)
			return -1;
		*(uint8_t *)pa = s[i];
	}
	return 0;
}

static int copy_str(uint32_t va, char *dst, uint32_t cap)
{
	uint32_t i;

	for (i = 0; i + 1u < cap; i++) {
		uint32_t pa = vm_user_pa(va + i);
		char c;

		if (pa == 0)
			return -1;
		c = *(char *)pa;
		dst[i] = c;
		if (c == '\0')
			return 0;
	}
	return -1;
}

static int elf_load(const char *name, uint32_t *entry)
{
	uint32_t len = 0;
	struct elf32_ehdr eh;
	uint16_t i;

	if (fs_read(name, filebuf, sizeof filebuf, &len) != 0)
		return -1;
	if (len < sizeof eh)
		return -1;
	memcpy(&eh, filebuf, sizeof eh);
	if (eh.ident[0] != 0x7f || eh.ident[1] != 'E' || eh.ident[2] != 'L' ||
	    eh.ident[3] != 'F')
		return -1;
	if (eh.ident[4] != 1 || eh.ident[5] != 1)
		return -1;
	if (eh.machine != 243 || eh.phentsize != 32)
		return -1;
	vm_user_clear();
	for (i = 0; i < eh.phnum; i++) {
		struct elf32_phdr ph;
		uint32_t off = eh.phoff + (uint32_t)i * 32u;
		uint32_t n;

		if (off + sizeof ph > len)
			return -1;
		memcpy(&ph, filebuf + off, sizeof ph);
		if (ph.type != 1)
			continue;
		if (ph.filesz > ph.memsz || ph.offset + ph.filesz > len)
			return -1;
		if (ph.vaddr < 0x10000u || ph.vaddr + ph.memsz < ph.vaddr)
			return -1;
		if (ph.vaddr + ph.memsz > 0x10000u + 16u * 4096u)
			return -1;
		for (n = 0; n < ph.filesz; n++) {
			uint32_t pa = vm_user_pa(ph.vaddr + n);

			if (pa == 0)
				return -1;
			*(uint8_t *)pa = filebuf[ph.offset + n];
		}
	}
	*entry = eh.entry;
	return 0;
}

static void resume(struct trapframe *tf, uint32_t entry)
{
	tf->sepc = entry;
	tf->x[2] = USER_STACK_TOP;
	tf->x[1] = 0;
	tf->x[10] = 0;
}

static int load_into(struct trapframe *tf, const char *name, int shell)
{
	uint32_t entry;

	if (elf_load(name, &entry) != 0)
		return -1;
	is_shell = shell;
	resume(tf, entry);
	return 0;
}

void shell_start(const struct fdt_info *fdt)
{
	uint32_t entry;

	if (virtio_init(fdt->virtio, fdt->nvirtio) != 0) {
		uart_puts("no block device\n");
		poweroff();
	}
	uart_puts("disk ");
	uart_hex(virtio_base());
	uart_putc('\n');
	if (fs_init() != 0) {
		uart_puts("no root\n");
		poweroff();
	}
	vm_map_user();
	if (elf_load("sh", &entry) != 0) {
		uart_puts("no sh\n");
		poweroff();
	}
	is_shell = 1;
	user_enter(entry, USER_STACK_TOP);
}

static int sys_write(uint32_t va, uint32_t len)
{
	uint32_t i;

	if (len > 256u)
		len = 256u;
	for (i = 0; i < len; i++) {
		uint32_t pa = vm_user_pa(va + i);

		if (pa == 0)
			return -1;
		uart_putc(*(char *)pa);
	}
	return (int)len;
}

static int sys_read(uint32_t va, uint32_t len)
{
	char buf[128];
	uint32_t n = 0;

	if (len == 0)
		return 0;
	if (len > sizeof buf)
		len = sizeof buf;
	for (;;) {
		int c = uart_getc();

		if (c == '\r')
			c = '\n';
		if (c == 0x7f || c == 0x08) {
			if (n > 0) {
				n--;
				uart_puts("\b \b");
			}
			continue;
		}
		if (c == '\n') {
			uart_putc('\n');
			break;
		}
		if (n + 1u < len) {
			buf[n++] = (char)c;
			uart_putc((char)c);
		}
	}
	if (copy_out(va, buf, n) != 0)
		return -1;
	return (int)n;
}

void syscall(struct trapframe *tf)
{
	uint32_t n = tf->x[17];
	uint32_t a0 = tf->x[10];
	uint32_t a1 = tf->x[11];
	uint32_t a2 = tf->x[12];
	int ret = -1;

	if (n == SYS_EXIT) {
		if (is_shell)
			poweroff();
		if (load_into(tf, "sh", 1) != 0)
			poweroff();
		return;
	}
	if (n == SYS_WRITE && a0 == 1)
		ret = sys_write(a1, a2);
	else if (n == SYS_READ && a0 == 0)
		ret = sys_read(a1, a2);
	else if (n == SYS_EXEC) {
		char path[28];

		if (copy_str(a0, path, sizeof path) == 0 &&
		    load_into(tf, path, 0) == 0)
			return;
	} else if (n == SYS_LIST) {
		char list[256];
		int len = fs_list(list, sizeof list);

		if (len >= 0 && (uint32_t)len <= a1 &&
		    copy_out(a0, list, (uint32_t)len) == 0)
			ret = len;
	} else if (n == SYS_RUN) {
		if (a0 == JOB_TIMER)
			run_timer();
		else if (a0 == JOB_SCHEDULE)
			run_schedule();
		else if (a0 == JOB_COPY)
			run_copy();
		else {
			tf->x[10] = (uint32_t)-1;
			tf->sepc += 4;
			return;
		}
		ret = 0;
	}
	tf->x[10] = (uint32_t)ret;
	tf->sepc += 4;
}
