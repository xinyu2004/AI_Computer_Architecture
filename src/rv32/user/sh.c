#include "user.h"

static int streq(const char *a, const char *b)
{
	while (*a != '\0' && *a == *b) {
		a++;
		b++;
	}
	return *a == '\0' && *b == '\0';
}

static void help(void)
{
	static const char text[] =
		"ls          programs on the disk\n"
		"timer       one timer interrupt\n"
		"schedule    task 1, then task 0\n"
		"copy_page   copy 4KiB\n"
		"exit        power off\n"
		"help        this list\n";

	sys_write(1, text, (int)sizeof text - 1);
}

int main(void)
{
	for (;;) {
		char buf[64];
		int n;

		sys_write(1, "$ ", 2);
		n = sys_read(0, buf, (int)sizeof buf - 1);
		if (n < 0)
			continue;
		buf[n] = '\0';
		if (buf[0] == '\0')
			continue;
		if (streq(buf, "exit"))
			sys_exit(0);
		if (streq(buf, "help")) {
			help();
			continue;
		}
		if (streq(buf, "ls")) {
			if (sys_exec("ls") < 0)
				sys_write(1, "not found\n", 10);
			continue;
		}
		if (streq(buf, "timer")) {
			sys_run(1);
			continue;
		}
		if (streq(buf, "schedule")) {
			sys_run(2);
			continue;
		}
		if (streq(buf, "copy_page")) {
			sys_run(3);
			continue;
		}
		sys_write(1, "not found\n", 10);
		sys_write(1, "try help\n", 9);
	}
}
