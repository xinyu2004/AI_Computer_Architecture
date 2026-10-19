#ifndef USER_H
#define USER_H

/*
 * svc #0. x8 is the number, x0-x2 are the arguments, and the kernel
 * returns in x0. Pointers are passed as 64-bit values. The numbers are
 * exit, write, read, exec, list, fork, wait, sleep, lookup.
 */
static inline long syscall(long num, long a, long b, long c)
{
	register long x0 asm("x0") = a;
	register long x1 asm("x1") = b;
	register long x2 asm("x2") = c;
	register long x8 asm("x8") = num;

	asm volatile("svc #0" : "+r"(x0) : "r"(x1), "r"(x2), "r"(x8) : "memory");
	return x0;
}

static inline void sys_exit(int code)
{
	syscall(0, code, 0, 0);
}

static inline int sys_write(int fd, const void *buf, int len)
{
	return (int)syscall(1, fd, (long)buf, len);
}

static inline int sys_read(int fd, void *buf, int len)
{
	return (int)syscall(2, fd, (long)buf, len);
}

static inline int sys_exec(const char *path, const char *arg)
{
	return (int)syscall(3, (long)path, (long)arg, 0);
}

static inline int sys_list(void *buf, int cap)
{
	return (int)syscall(4, (long)buf, cap, 0);
}

static inline int sys_fork(void)
{
	return (int)syscall(5, 0, 0, 0);
}

static inline int sys_wait(void)
{
	return (int)syscall(6, 0, 0, 0);
}

static inline int sys_sleep(int ms)
{
	return (int)syscall(7, ms, 0, 0);
}

static inline int sys_lookup(const char *path)
{
	return (int)syscall(8, (long)path, 0, 0);
}

#endif
