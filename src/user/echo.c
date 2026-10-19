#include "user.h"

/*
 * argv[1] is the string placed on the user stack by exec. A second
 * echo of the same text reads the program again; the kernel's sector
 * cache is what makes that read hit.
 */

int main(int argc, char **argv)
{
	const char *s;
	int n;

	if (argc >= 2) {
		s = argv[1];
		n = 0;
		while (s[n] != '\0')
			n++;
		if (n > 0)
			sys_write(1, s, n);
	}
	sys_write(1, "\n", 1);
	sys_exit(0);
	return 0;
}
