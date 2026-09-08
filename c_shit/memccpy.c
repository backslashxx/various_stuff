void *memccpy(void *restrict dest, const void *restrict src, int c, size_t n)
{
	if (!n)
		return nullptr;

	unsigned char *s = (unsigned char *)src;
	unsigned char target = (unsigned char)c;

	unsigned char *p = __builtin_memchr(s, target, n);
	if (p) {
		size_t len = (size_t)(p - s) + 1;
		__builtin_memcpy(dest, src, len);
		return (void*)((char *)dest + len);
	}

	__builtin_memcpy(dest, src, n);
	return nullptr;
}
