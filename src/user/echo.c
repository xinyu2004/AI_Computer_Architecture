#include "user.h"

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
