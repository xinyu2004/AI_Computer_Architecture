#include "user.h"

/* The argument is a number of milliseconds. sys_sleep waits that long. */

int main(int argc, char **argv)
{
	const char *s;
	int ms;

	if (argc < 2 || argv[1][0] == '\0') {
		sys_write(1, "sleep: ms\n", 10);
		sys_exit(0);
		return 0;
	}
	ms = 0;
	for (s = argv[1]; *s != '\0'; s++) {
		if (*s < '0' || *s > '9') {
			sys_write(1, "sleep: ms\n", 10);
			sys_exit(0);
			return 0;
		}
		ms = ms * 10 + (*s - '0');
	}
	sys_sleep(ms);
	sys_exit(0);
	return 0;
}
