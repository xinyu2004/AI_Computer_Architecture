#include "armv7.h"

void *memset(void *dst, int c, unsigned n)
{
	uint8_t *d = dst;

	while (n-- > 0)
		*d++ = (uint8_t)c;
	return dst;
}

void copy_page(uint32_t *dst, const uint32_t *src)
{
	uint32_t i;

	for (i = 0; i < 1024; i++)
		dst[i] = src[i];
}

void *memcpy(void *dst, const void *src, unsigned n)
{
	uint8_t *d = dst;
	const uint8_t *s = src;

	while (n-- > 0)
		*d++ = *s++;
	return dst;
}
