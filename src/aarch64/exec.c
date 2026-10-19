#include "aarch64.h"

/*
 * sh is loaded from the disk and entered in user mode. It stays there.
 * fork copies the shell's stack page. exec loads another program and
 * swtch enters it. exit swtch's back and the saved shell pages are
 * restored, so sh is not read from the disk again.
 *
 * svc leaves ELR_EL1 on the next instruction. The call number is x8.
 * Arguments are x0, x1, and x2.
 */

#define SYS_EXIT 0u
#define SYS_WRITE 1u
#define SYS_READ 2u
#define SYS_EXEC 3u
#define SYS_LIST 4u
#define SYS_FORK 5u
#define SYS_WAIT 6u
#define SYS_SLEEP 7u
#define SYS_LOOKUP 8u

char current_name[28] = "sh";
struct trapframe child_tf;

static int have_child;
static int child_exited;
static int child_status;
static int in_child;
static uint8_t filebuf[32 * 1024];
static uint8_t child_stack[4096] __attribute__((aligned(4096)));
static uint8_t child_kstack[4096] __attribute__((aligned(16)));
static struct context shell_ctx;
static struct context child_ctx;

struct elf64_ehdr {
	uint8_t ident[16];
	uint16_t type;
	uint16_t machine;
	uint32_t version;
	uint64_t entry;
	uint64_t phoff;
	uint64_t shoff;
	uint32_t flags;
	uint16_t ehsize;
	uint16_t phentsize;
	uint16_t phnum;
	uint16_t shentsize;
	uint16_t shnum;
	uint16_t shstrndx;
};

struct elf64_phdr {
	uint32_t type;
	uint32_t flags;
	uint64_t offset;
	uint64_t vaddr;
	uint64_t paddr;
	uint64_t filesz;
	uint64_t memsz;
	uint64_t align;
};

static int copy_out(uint64_t va, const void *src, uint32_t len)
{
	uint32_t i;
	const uint8_t *s = src;

	for (i = 0; i < len; i++) {
		uint64_t pa = vm_user_pa(va + i);

		if (pa == 0)
			return -1;
		*(uint8_t *)pa = s[i];
	}
	return 0;
}

static int copy_str(uint64_t va, char *dst, uint32_t cap)
{
	uint32_t i;

	for (i = 0; i + 1u < cap; i++) {
		uint64_t pa = vm_user_pa(va + i);
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

static int elf_load(const char *name, uint64_t *entry)
{
	uint32_t len = 0;
	struct elf64_ehdr eh;
	uint16_t i;

	if (fs_read(name, filebuf, sizeof filebuf, &len) != 0)
		return -1;
	if (len < sizeof eh)
		return -1;
	memcpy(&eh, filebuf, sizeof eh);
	if (eh.ident[0] != 0x7f || eh.ident[1] != 'E' || eh.ident[2] != 'L' ||
	    eh.ident[3] != 'F')
		return -1;
	/*
	 * ELF64, little-endian. Machine 183 is AArch64. A program header
	 * is 56 bytes, and flags comes before offset. Type 1 is a LOAD
	 * segment. Bytes are copied through the physical address of the
	 * user page.
	 */
	if (eh.ident[4] != 2 || eh.ident[5] != 1)
		return -1;
	if (eh.machine != 183 || eh.phentsize != 56)
		return -1;
	vm_user_clear();
	for (i = 0; i < eh.phnum; i++) {
		struct elf64_phdr ph;
		uint64_t off = eh.phoff + (uint64_t)i * 56u;
		uint64_t n;

		if (off + sizeof ph > len)
			return -1;
		memcpy(&ph, filebuf + off, sizeof ph);
		if (ph.type != 1)
			continue;
		if (ph.filesz > ph.memsz || ph.offset + ph.filesz > len)
			return -1;
		if (ph.vaddr < 0x10000ull || ph.vaddr + ph.memsz < ph.vaddr)
			return -1;
		if (ph.vaddr + ph.memsz > 0x10000ull + 16u * 4096u)
			return -1;
		for (n = 0; n < ph.filesz; n++) {
			uint64_t pa = vm_user_pa(ph.vaddr + n);

			if (pa == 0)
				return -1;
			*(uint8_t *)pa = filebuf[ph.offset + n];
		}
	}
	*entry = eh.entry;
	cpu_dsb();
	asm volatile("isb" ::: "memory");
	return 0;
}

	/*
	 * The disk is found, the directory is read, and the user window is
	 * mapped. sh is loaded at 0x10000. The timer is armed, then eret
	 * enters the shell with the stack at 0x81000.
	 */
void shell_start(const struct fdt_info *fdt)
{
	uint64_t entry;

	if (virtio_init(fdt) != 0) {
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
	timer_arm(fdt->timebase);
	user_enter(entry, USER_STACK_TOP);
}

static void set_name(const char *s)
{
	uint32_t i;

	for (i = 0; i + 1u < sizeof current_name && s[i] != '\0'; i++)
		current_name[i] = s[i];
	current_name[i] = '\0';
}

static void child_start(void)
{
	user_resume();
}

static uint64_t push_str(uint64_t sp, const char *s, uint64_t *at)
{
	uint32_t n = 0;

	while (s[n] != '\0')
		n++;
	n++;
	sp -= n;
	if (copy_out(sp, s, n) != 0)
		return 0;
	*at = sp;
	return sp;
}

static int write_u64(uint64_t va, uint64_t value)
{
	return copy_out(va, &value, 8);
}

	/*
	 * Strings go at the top of the user stack, then the argv pointer
	 * array. Each pointer is 8 bytes. AAPCS64 wants sp 16-byte aligned,
	 * so the pointer array is padded out to a multiple of 16.
	 */
static int place_args(const char *name, const char *arg, uint64_t *argc,
		      uint64_t *argv, uint64_t *sp_out)
{
	uint64_t sp = USER_STACK_TOP;
	uint64_t s0;
	uint64_t s1 = 0;
	uint64_t nargc = 1;
	uint64_t bytes;

	sp = push_str(sp, name, &s0);
	if (sp == 0)
		return -1;
	if (arg[0] != '\0') {
		sp = push_str(sp, arg, &s1);
		if (sp == 0)
			return -1;
		nargc = 2;
	}
	sp &= ~15ull;
	bytes = (nargc + 1u) * 8u;
	if ((bytes & 15u) != 0)
		bytes += 8u;
	sp -= bytes;
	if (sp < 0x80000ull)
		return -1;
	if (write_u64(sp, s0) != 0)
		return -1;
	if (nargc == 2 && write_u64(sp + 8u, s1) != 0)
		return -1;
	if (write_u64(sp + nargc * 8u, 0) != 0)
		return -1;
	*argc = nargc;
	*argv = sp;
	*sp_out = sp;
	return 0;
}

	/*
	 * x0 and x1 are argc and argv. The pointers are 8 bytes, and
	 * place_args keeps SP_EL0 16-byte aligned. SPSR 0 is EL0t.
	 * ELR_EL1 is the ELF entry.
	 */
static void fill_child_frame(uint64_t entry, uint64_t sp, uint64_t argc,
			     uint64_t argv)
{
	uint32_t i;

	for (i = 0; i < 31; i++)
		child_tf.x[i] = 0;
	child_tf.x[0] = argc;
	child_tf.x[1] = argv;
	child_tf.sp = sp;
	child_tf.elr = entry;
	child_tf.spsr = 0;
}

	/*
	 * One page, 512 words of 64 bits, from the shell stack at 0x80000
	 * into child_stack. The shell's code pages are saved so exec can
	 * put them back. The child does not run until exec calls swtch.
	 */
static int sys_fork(void)
{
	if (have_child)
		return -1;
	copy_page((uint64_t *)child_stack, (uint64_t *)vm_user_pa(0x80000u));
	vm_save_code();
	have_child = 1;
	return 1;
}

	/*
	 * The new program replaces the user code. swtch saves the shell
	 * and resumes child_start, which erets into that program. The
	 * child's exit swtch's back to the instruction after swtch, and
	 * the saved shell pages are copied back. sh is not read again.
	 * Both tasks stay at EL1 between the two returns.
	 */
static int sys_exec(const char *name, const char *arg)
{
	uint64_t entry;
	uint64_t argc;
	uint64_t argv;
	uint64_t sp;

	if (!have_child)
		return -1;
	if (elf_load(name, &entry) != 0) {
		vm_restore_code();
		have_child = 0;
		return -1;
	}
	vm_set_stack(child_stack);
	if (place_args(name, arg, &argc, &argv, &sp) != 0) {
		vm_use_shell_stack();
		vm_restore_code();
		have_child = 0;
		return -1;
	}
	fill_child_frame(entry, sp, argc, argv);
	set_name(name);
	child_ctx.x30 = (uint64_t)child_start;
	child_ctx.sp = (uint64_t)child_kstack + sizeof child_kstack;
	in_child = 1;
	swtch(&shell_ctx, &child_ctx);
	in_child = 0;
	vm_use_shell_stack();
	vm_restore_code();
	cpu_dsb();
	asm volatile("isb" ::: "memory");
	have_child = 0;
	set_name("sh");
	return 0;
}

static int sys_write(uint64_t va, uint64_t len)
{
	uint64_t i;

	if (len > 256u)
		len = 256u;
	for (i = 0; i < len; i++) {
		uint64_t pa = vm_user_pa(va + i);

		if (pa == 0)
			return -1;
		uart_putc(*(char *)pa);
	}
	return (int)len;
}

static int sys_read(uint64_t va, uint64_t len)
{
	char buf[128];
	int n;

	if (len == 0)
		return 0;
	if (len > sizeof buf)
		len = sizeof buf;
	n = read_line(buf, (uint32_t)len);
	if (n < 0)
		return -1;
	if (copy_out(va, buf, (uint32_t)n) != 0)
		return -1;
	return n;
}

	/*
	 * x8 is the number. x0-x2 are the arguments. The return value
	 * goes back in x0. ELR_EL1 already addresses the next instruction,
	 * so it is left alone.
	 */
void syscall(struct trapframe *tf)
{
	uint64_t n = tf->x[8];
	uint64_t a0 = tf->x[0];
	uint64_t a1 = tf->x[1];
	uint64_t a2 = tf->x[2];
	long ret = -1;

	if (n == SYS_EXIT) {
		if (!in_child)
			poweroff();
		child_status = (int)a0;
		child_exited = 1;
		swtch(&child_ctx, &shell_ctx);
		return;
	}
	if (n == SYS_WRITE && a0 == 1)
		ret = sys_write(a1, a2);
	else if (n == SYS_READ && a0 == 0)
		ret = sys_read(a1, a2);
	else if (n == SYS_EXEC) {
		char path[28];
		char arg[64];

		if (copy_str(a0, path, sizeof path) == 0 &&
		    copy_str(a1, arg, sizeof arg) == 0)
			ret = sys_exec(path, arg);
	} else if (n == SYS_LIST) {
		char list[256];
		int len = fs_list(list, sizeof list);

		if (len >= 0 && (uint64_t)len <= a1 &&
		    copy_out(a0, list, (uint32_t)len) == 0)
			ret = len;
	} else if (n == SYS_FORK) {
		ret = sys_fork();
	} else if (n == SYS_WAIT) {
		if (child_exited) {
			ret = child_status;
			child_exited = 0;
		}
	} else if (n == SYS_SLEEP) {
		sleep_ms((uint32_t)a0);
		ret = 0;
	} else if (n == SYS_LOOKUP) {
		char path[28];

		if (copy_str(a0, path, sizeof path) == 0)
			ret = fs_exists(path) ? 0 : -1;
	}
	tf->x[0] = (uint64_t)ret;
}
