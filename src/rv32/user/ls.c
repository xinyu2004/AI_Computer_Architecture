#include "user.h"

int main(void)
{
	char buf[128];
	int n = sys_list(buf, (int)sizeof buf - 1);

	if (n > 0)
		sys_write(1, buf, n);
	sys_exit(0);
	return 0;
}
