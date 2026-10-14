/*
 *  linux/tools/build.c
 *
 *  (C) 1991  Linus Torvalds
 *
 * Packs bootsect, setup, and the kernel into the floppy image.
 * The inputs are raw binaries (objcopy -O binary / ld --oformat binary).
 * The 1991 a.out and Minix headers are no longer produced by the toolchain.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>

#define SYS_SIZE 0x3000
#define SETUP_SECTS 4
#define STRINGIFY(x) #x

static void die(const char *str)
{
	fprintf(stderr, "%s\n", str);
	exit(1);
}

static void usage(void)
{
	die("Usage: build bootsect setup system [rootdev] [> image]");
}

static int read_all(const char *path, unsigned char **out)
{
	int fd, n, got;
	struct stat sb;
	unsigned char *buf;

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		perror(path);
		die("Unable to open input");
	}
	if (fstat(fd, &sb) < 0)
		die("Unable to stat input");
	if (sb.st_size <= 0 || sb.st_size > 8 * 1024 * 1024)
		die("Input has an unexpected size");
	buf = malloc(sb.st_size);
	if (!buf)
		die("Out of memory");
	got = 0;
	while (got < sb.st_size) {
		n = read(fd, buf + got, sb.st_size - got);
		if (n <= 0)
			die("Unable to read input");
		got += n;
	}
	close(fd);
	*out = buf;
	return got;
}

static void write_exact(const unsigned char *buf, int n)
{
	if (write(1, buf, n) != n)
		die("Write call failed");
}

static void root_device(const char *spec, int *major_root, int *minor_root)
{
	struct stat sb;
	int maj, min;

	if (!strcmp(spec, "FLOPPY")) {
		*major_root = 0;
		*minor_root = 0;
		return;
	}
	if (sscanf(spec, "%d,%d", &maj, &min) == 2) {
		*major_root = maj;
		*minor_root = min;
		return;
	}
	if (stat(spec, &sb)) {
		perror(spec);
		die("Couldn't stat root device.");
	}
	*major_root = (int) major(sb.st_rdev);
	*minor_root = (int) minor(sb.st_rdev);
}

int main(int argc, char **argv)
{
	int i, major_root, minor_root, n;
	unsigned char *boot, *setup, *system, zero[1024];

	if ((argc != 4) && (argc != 5))
		usage();
	if (argc == 5)
		root_device(argv[4], &major_root, &minor_root);
	else {
		major_root = 3;
		minor_root = 6;
	}
	fprintf(stderr, "Root device is (%d, %d)\n", major_root, minor_root);
	if ((major_root != 2) && (major_root != 3) && (major_root != 0)) {
		fprintf(stderr, "Illegal root device (major = %d)\n", major_root);
		die("Bad root device --- major #");
	}

	n = read_all(argv[1], &boot);
	fprintf(stderr, "Boot sector %d bytes.\n", n);
	if (n != 512)
		die("Boot block must be exactly 512 bytes");
	if (*(unsigned short *)(boot + 510) != 0xAA55)
		die("Boot block hasn't got boot flag (0xAA55)");
	boot[508] = (unsigned char) minor_root;
	boot[509] = (unsigned char) major_root;
	write_exact(boot, 512);

	n = read_all(argv[2], &setup);
	fprintf(stderr, "Setup is %d bytes.\n", n);
	if (n > SETUP_SECTS * 512)
		die("Setup exceeds " STRINGIFY(SETUP_SECTS)
			" sectors - rewrite build/boot/setup");
	write_exact(setup, n);
	memset(zero, 0, sizeof zero);
	while (n < SETUP_SECTS * 512) {
		i = SETUP_SECTS * 512 - n;
		if (i > (int) sizeof zero)
			i = (int) sizeof zero;
		write_exact(zero, i);
		n += i;
	}

	n = read_all(argv[3], &system);
	fprintf(stderr, "System is %d bytes.\n", n);
	if (n > SYS_SIZE * 16)
		die("System is too big");
	write_exact(system, n);
	/*
	 * bootsect.s reads SYSSIZE paragraphs from the floppy, past the
	 * last byte of the kernel. A short file makes Bochs hit EOF on
	 * those sectors. The extra bytes are zeros.
	 */
	memset(zero, 0, sizeof zero);
	while (n < SYS_SIZE * 16) {
		i = SYS_SIZE * 16 - n;
		if (i > (int) sizeof zero)
			i = (int) sizeof zero;
		write_exact(zero, i);
		n += i;
	}
	return 0;
}
