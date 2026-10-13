#include "rv32.h"

void *memset(void *dst, int c, unsigned n)
{
	uint8_t *d = dst;

	while (n-- > 0)
		*d++ = (uint8_t)c;
	return dst;
}

void *memcpy(void *dst, const void *src, unsigned n)
{
	uint8_t *d = dst;
	const uint8_t *s = src;

	while (n-- > 0)
		*d++ = *s++;
	return dst;
}
