#ifndef USER_H
#define USER_H

static inline int syscall(int num, int a, int b, int c)
{
	register int a0 asm("a0") = a;
	register int a1 asm("a1") = b;
	register int a2 asm("a2") = c;
	register int a7 asm("a7") = num;

	asm volatile("ecall" : "+r"(a0) : "r"(a1), "r"(a2), "r"(a7) : "memory");
	return a0;
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

static inline int sys_exec(const char *path)
{
	return syscall(3, (int)path, 0, 0);
}

static inline int sys_list(void *buf, int cap)
{
	return syscall(4, (int)buf, cap, 0);
}

static inline int sys_run(int job)
{
	return syscall(5, job, 0, 0);
}

#endif
