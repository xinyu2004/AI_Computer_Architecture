#ifndef _STRING_H_
#define _STRING_H_

#ifndef NULL
#define NULL ((void *) 0)
#endif

#ifndef _SIZE_T
#define _SIZE_T
typedef unsigned int size_t;
#endif

extern char * strerror(int errno);

/*
 * This string-include defines all string functions as inline
 * functions. Use gcc. It also assumes ds=es=data space, this should be
 * normal. Most of the string-functions are rather heavily hand-optimized,
 * see especially strtok,strstr,str[c]spn. They should work, but are not
 * very easy to understand. Everything is done entirely within the register
 * set, making the functions fast and clean. String instructions have been
 * used through-out, making for "slightly" unclear code :-)
 *
 *		(C) 1991 Linus Torvalds
 */
 
static inline char * strcpy(char * dest,const char *src)
{
	char *d = dest;
	const char *s = src;
	/* lodsb/stosb advance esi and edi. The returned pointer is dest. */
	__asm__ volatile ("cld\n"
		"1:\tlodsb\n\t"
		"stosb\n\t"
		"testb %%al,%%al\n\t"
		"jne 1b"
		: "+S" (s), "+D" (d)
		:
		: "ax", "cc", "memory");
	return dest;
}

static inline char * strncpy(char * dest,const char *src,int count)
{
	char *d = dest;
	const char *s = src;
	int n = count;
	__asm__ volatile ("cld\n"
		"1:\tdecl %2\n\t"
		"js 2f\n\t"
		"lodsb\n\t"
		"stosb\n\t"
		"testb %%al,%%al\n\t"
		"jne 1b\n\t"
		"rep\n\t"
		"stosb\n"
		"2:"
		: "+S" (s), "+D" (d), "+c" (n)
		:
		: "ax", "cc", "memory");
	return dest;
}

static inline char * strcat(char * dest,const char * src)
{
	char *d = dest;
	const char *s = src;
	int a = 0, n = -1;
	__asm__ volatile ("cld\n\t"
		"repne\n\t"
		"scasb\n\t"
		"decl %1\n"
		"1:\tlodsb\n\t"
		"stosb\n\t"
		"testb %%al,%%al\n\t"
		"jne 1b"
		: "+S" (s), "+D" (d), "+a" (a), "+c" (n)
		:
		: "cc", "memory");
	return dest;
}

static inline char * strncat(char * dest,const char * src,int count)
{
	char *d = dest;
	const char *s = src;
	int a = 0, n = -1;
	__asm__ volatile ("cld\n\t"
		"repne\n\t"
		"scasb\n\t"
		"decl %1\n\t"
		"movl %4,%3\n"
		"1:\tdecl %3\n\t"
		"js 2f\n\t"
		"lodsb\n\t"
		"stosb\n\t"
		"testb %%al,%%al\n\t"
		"jne 1b\n"
		"2:\txorl %2,%2\n\t"
		"stosb"
		: "+S" (s), "+D" (d), "+a" (a), "+c" (n)
		: "g" (count)
		: "cc", "memory");
	return dest;
}

static inline int strcmp(const char * cs,const char * ct)
{
	int __res;
	const char *a = cs, *b = ct;
	__asm__ volatile ("cld\n"
		"1:\tlodsb\n\t"
		"scasb\n\t"
		"jne 2f\n\t"
		"testb %%al,%%al\n\t"
		"jne 1b\n\t"
		"xorl %%eax,%%eax\n\t"
		"jmp 3f\n"
		"2:\tmovl $1,%%eax\n\t"
		"jl 3f\n\t"
		"negl %%eax\n"
		"3:"
		: "=a" (__res), "+D" (a), "+S" (b)
		:
		: "cc");
	return __res;
}

static inline int strncmp(const char * cs,const char * ct,int count)
{
	int __res, n = count;
	const char *a = cs, *b = ct;
	__asm__ volatile ("cld\n"
		"1:\tdecl %3\n\t"
		"js 2f\n\t"
		"lodsb\n\t"
		"scasb\n\t"
		"jne 3f\n\t"
		"testb %%al,%%al\n\t"
		"jne 1b\n"
		"2:\txorl %%eax,%%eax\n\t"
		"jmp 4f\n"
		"3:\tmovl $1,%%eax\n\t"
		"jl 4f\n\t"
		"negl %%eax\n"
		"4:"
		: "=a" (__res), "+D" (a), "+S" (b), "+c" (n)
		:
		: "cc");
	return __res;
}

static inline char * strchr(const char * s,char c)
{
	char * __res;
	const char *p = s;
	__asm__ volatile ("cld\n\t"
		"movb %%al,%%ah\n"
		"1:\tlodsb\n\t"
		"cmpb %%ah,%%al\n\t"
		"je 2f\n\t"
		"testb %%al,%%al\n\t"
		"jne 1b\n\t"
		"movl $1,%1\n"
		"2:\tmovl %1,%0\n\t"
		"decl %0"
		: "=a" (__res), "+S" (p)
		: "0" (c)
		: "cc");
	return __res;
}

static inline char * strrchr(const char * s,char c)
{
	char * __res;
	const char *p = s;
	int ch = c;
	__asm__ volatile ("cld\n\t"
		"movb %%al,%%ah\n"
		"1:\tlodsb\n\t"
		"cmpb %%ah,%%al\n\t"
		"jne 2f\n\t"
		"movl %%esi,%0\n\t"
		"decl %0\n"
		"2:\ttestb %%al,%%al\n\t"
		"jne 1b"
		: "=d" (__res), "+S" (p), "+a" (ch)
		: "0" (0)
		: "cc");
	return __res;
}

static inline int strspn(const char * cs, const char * ct)
{
	char * __res;
	int zero = 0, neg = -1;
	__asm__ volatile ("cld\n\t"
		"movl %4,%%edi\n\t"
		"repne\n\t"
		"scasb\n\t"
		"notl %%ecx\n\t"
		"decl %%ecx\n\t"
		"movl %%ecx,%%edx\n"
		"1:\tlodsb\n\t"
		"testb %%al,%%al\n\t"
		"je 2f\n\t"
		"movl %4,%%edi\n\t"
		"movl %%edx,%%ecx\n\t"
		"repne\n\t"
		"scasb\n\t"
		"je 1b\n"
		"2:\tdecl %0"
		: "=S" (__res), "+a" (zero), "+c" (neg)
		: "0" (cs), "g" (ct)
		: "dx", "di", "cc");
	return __res-cs;
}

static inline int strcspn(const char * cs, const char * ct)
{
	char * __res;
	int zero = 0, neg = -1;
	__asm__ volatile ("cld\n\t"
		"movl %4,%%edi\n\t"
		"repne\n\t"
		"scasb\n\t"
		"notl %%ecx\n\t"
		"decl %%ecx\n\t"
		"movl %%ecx,%%edx\n"
		"1:\tlodsb\n\t"
		"testb %%al,%%al\n\t"
		"je 2f\n\t"
		"movl %4,%%edi\n\t"
		"movl %%edx,%%ecx\n\t"
		"repne\n\t"
		"scasb\n\t"
		"jne 1b\n"
		"2:\tdecl %0"
		: "=S" (__res), "+a" (zero), "+c" (neg)
		: "0" (cs), "g" (ct)
		: "dx", "di", "cc");
	return __res-cs;
}

static inline char * strpbrk(const char * cs,const char * ct)
{
	char * __res;
	int zero = 0, neg = -1;
	__asm__ volatile ("cld\n\t"
		"movl %4,%%edi\n\t"
		"repne\n\t"
		"scasb\n\t"
		"notl %%ecx\n\t"
		"decl %%ecx\n\t"
		"movl %%ecx,%%edx\n"
		"1:\tlodsb\n\t"
		"testb %%al,%%al\n\t"
		"je 2f\n\t"
		"movl %4,%%edi\n\t"
		"movl %%edx,%%ecx\n\t"
		"repne\n\t"
		"scasb\n\t"
		"jne 1b\n\t"
		"decl %0\n\t"
		"jmp 3f\n"
		"2:\txorl %0,%0\n"
		"3:"
		: "=S" (__res), "+a" (zero), "+c" (neg)
		: "0" (cs), "g" (ct)
		: "dx", "di", "cc");
	return __res;
}

static inline char * strstr(const char * cs,const char * ct)
{
	char * __res;
	int neg = -1;
	const char *p = cs;
	__asm__ volatile ("cld\n\t"
		"movl %4,%%edi\n\t"
		"repne\n\t"
		"scasb\n\t"
		"notl %%ecx\n\t"
		"decl %%ecx\n\t"	/* NOTE! This also sets Z if searchstring='' */
		"movl %%ecx,%%edx\n"
		"1:\tmovl %4,%%edi\n\t"
		"movl %%esi,%%eax\n\t"
		"movl %%edx,%%ecx\n\t"
		"repe\n\t"
		"cmpsb\n\t"
		"je 2f\n\t"		/* also works for empty string, see above */
		"xchgl %%eax,%%esi\n\t"
		"incl %%esi\n\t"
		"cmpb $0,-1(%%eax)\n\t"
		"jne 1b\n\t"
		"xorl %%eax,%%eax\n\t"
		"2:"
		: "=a" (__res), "+c" (neg), "+S" (p)
		: "0" (0), "g" (ct)
		: "dx", "di", "cc");
	return __res;
}

static inline int strlen(const char * s)
{
	int __res;
	const char *p = s;
	int z = 0;
	__asm__ volatile ("cld\n\t"
		"repne\n\t"
		"scasb\n\t"
		"notl %0\n\t"
		"decl %0"
		: "=c" (__res), "+D" (p), "+a" (z)
		: "0" (~0)
		: "cc");
	return __res;
}

extern char * ___strtok;

static inline char * strtok(char * s,const char * ct)
{
register char * __res ;
__asm__("testl %1,%1\n\t"
	"jne 1f\n\t"
	"testl %0,%0\n\t"
	"je 8f\n\t"
	"movl %0,%1\n"
	"1:\txorl %0,%0\n\t"
	"movl $-1,%%ecx\n\t"
	"xorl %%eax,%%eax\n\t"
	"cld\n\t"
	"movl %4,%%edi\n\t"
	"repne\n\t"
	"scasb\n\t"
	"notl %%ecx\n\t"
	"decl %%ecx\n\t"
	"je 7f\n\t"			/* empty delimeter-string */
	"movl %%ecx,%%edx\n"
	"2:\tlodsb\n\t"
	"testb %%al,%%al\n\t"
	"je 7f\n\t"
	"movl %4,%%edi\n\t"
	"movl %%edx,%%ecx\n\t"
	"repne\n\t"
	"scasb\n\t"
	"je 2b\n\t"
	"decl %1\n\t"
	"cmpb $0,(%1)\n\t"
	"je 7f\n\t"
	"movl %1,%0\n"
	"3:\tlodsb\n\t"
	"testb %%al,%%al\n\t"
	"je 5f\n\t"
	"movl %4,%%edi\n\t"
	"movl %%edx,%%ecx\n\t"
	"repne\n\t"
	"scasb\n\t"
	"jne 3b\n\t"
	"decl %1\n\t"
	"cmpb $0,(%1)\n\t"
	"je 5f\n\t"
	"movb $0,(%1)\n\t"
	"incl %1\n\t"
	"jmp 6f\n"
	"5:\txorl %1,%1\n"
	"6:\tcmpb $0,(%0)\n\t"
	"jne 7f\n\t"
	"xorl %0,%0\n"
	"7:\ttestl %0,%0\n\t"
	"jne 8f\n\t"
	"movl %0,%1\n"
	"8:"
	: "=b" (__res), "=S" (___strtok)
	: "0" (___strtok), "1" (s), "g" (ct)
	: "ax", "cx", "dx", "di", "cc", "memory");
return __res;
}

static inline void * memcpy(void * dest,const void * src, int n)
{
	void *d = dest;
	const void *s = src;
	int count = n;
	/*
	 * edi/esi/ecx change. Callers such as read_inode keep the
	 * destination pointer live across this copy.
	 */
	__asm__ volatile ("cld\n\t"
		"rep\n\t"
		"movsb"
		: "+D" (d), "+S" (s), "+c" (count)
		:
		: "memory");
	return dest;
}

static inline void * memmove(void * dest,const void * src, int n)
{
	void *d;
	const void *s;
	int count = n;
	if (dest<src) {
		d = dest;
		s = src;
		__asm__ volatile ("cld\n\t"
			"rep\n\t"
			"movsb"
			: "+D" (d), "+S" (s), "+c" (count)
			:
			: "memory");
	} else {
		d = (char *)dest + n - 1;
		s = (const char *)src + n - 1;
		__asm__ volatile ("std\n\t"
			"rep\n\t"
			"movsb\n\t"
			"cld"
			: "+D" (d), "+S" (s), "+c" (count)
			:
			: "memory");
	}
	return dest;
}

static inline int memcmp(const void * cs,const void * ct,int count)
{
	int __res;
	const void *a = cs, *b = ct;
	int n = count;
	__asm__ volatile ("cld\n\t"
		"repe\n\t"
		"cmpsb\n\t"
		"je 1f\n\t"
		"movl $1,%%eax\n\t"
		"jl 1f\n\t"
		"negl %%eax\n"
		"1:"
		: "=a" (__res), "+D" (a), "+S" (b), "+c" (n)
		:
		: "cc");
	return __res;
}

static inline void * memchr(const void * cs,char c,int count)
{
	void * __res;
	int n = count;
	int ch = c;
	if (!count)
		return NULL;
	__asm__ volatile ("cld\n\t"
		"repne\n\t"
		"scasb\n\t"
		"je 1f\n\t"
		"movl $1,%0\n"
		"1:\tdecl %0"
		: "=D" (__res), "+c" (n), "+a" (ch)
		: "0" (cs)
		: "cc");
	return __res;
}

static inline void * memset(void * s,char c,int count)
{
	void *d = s;
	int n = count;
	int ch = c;
	__asm__ volatile ("cld\n\t"
		"rep\n\t"
		"stosb"
		: "+D" (d), "+c" (n), "+a" (ch)
		:
		: "memory");
	return s;
}

#endif
