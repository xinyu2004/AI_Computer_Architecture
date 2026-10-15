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
	char buf[128];
	int n = sys_list(buf, (int)sizeof buf - 1);

	sys_write(1, "programs\n", 9);
	if (n > 0)
		sys_write(1, buf, n);
	sys_write(1, "exit        power off\n", 20);
	sys_write(1, "sleep N     milliseconds\n", 24);
}

static void launch(const char *name, const char *arg)
{
	if (sys_lookup(name) != 0) {
		sys_write(1, "not found\n", 10);
		return;
	}
	if (sys_fork() < 0)
		return;
	if (sys_exec(name, arg) == 0)
		sys_wait();
}

int main(void)
{
	for (;;) {
		char buf[64];
		char name[28];
		char arg[64];
		int n;
		int i;
		int j;

		sys_write(1, "$ ", 2);
		n = sys_read(0, buf, (int)sizeof buf - 1);
		if (n < 0)
			continue;
		buf[n] = '\0';
		i = 0;
		while (buf[i] == ' ')
			i++;
		if (buf[i] == '\0')
			continue;
		j = 0;
		while (buf[i] != '\0' && buf[i] != ' ' && j < (int)sizeof name - 1)
			name[j++] = buf[i++];
		name[j] = '\0';
		while (buf[i] == ' ')
			i++;
		j = 0;
		while (buf[i] != '\0' && j < (int)sizeof arg - 1)
			arg[j++] = buf[i++];
		arg[j] = '\0';
		if (streq(name, "exit"))
			sys_exit(0);
		if (streq(name, "help")) {
			help();
			continue;
		}
		launch(name, arg);
	}
}
