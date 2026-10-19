#ifndef USER_H
#define USER_H

static inline long syscall(long num, long a, long b, long c)
{
	register long a0 asm("a0") = a;
	register long a1 asm("a1") = b;
	register long a2 asm("a2") = c;
	register long a7 asm("a7") = num;

	asm volatile("ecall" : "+r"(a0) : "r"(a1), "r"(a2), "r"(a7) : "memory");
	return a0;
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
	return syscall(5, 0, 0, 0);
}

static inline int sys_wait(void)
{
	return syscall(6, 0, 0, 0);
}

static inline int sys_sleep(int ms)
{
	return syscall(7, ms, 0, 0);
}

static inline int sys_lookup(const char *path)
{
	return (int)syscall(8, (long)path, 0, 0);
}

#endif
