#ifndef USER_H
#define USER_H

static inline int syscall(int num, int a, int b, int c)
{
	register int r0 asm("r0") = a;
	register int r1 asm("r1") = b;
	register int r2 asm("r2") = c;
	register int r7 asm("r7") = num;

	asm volatile("svc #0" : "+r"(r0) : "r"(r1), "r"(r2), "r"(r7) : "memory");
	return r0;
}

static inline void sys_exit(int code)
{
	syscall(0, code, 0, 0);
}

static inline int sys_write(int fd, const void *buf, int len)
{
	return syscall(1, fd, (int)buf, len);
}

static inline int sys_read(int fd, void *buf, int len)
{
	return syscall(2, fd, (int)buf, len);
}

static inline int sys_exec(const char *path, const char *arg)
{
	return syscall(3, (int)path, (int)arg, 0);
}

static inline int sys_list(void *buf, int cap)
{
	return syscall(4, (int)buf, cap, 0);
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
	return syscall(8, (int)path, 0, 0);
}

#endif
