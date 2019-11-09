#include <stdio.h>
#include <stdlib.h>	// atof
#include <string.h>
#include "utils.h"

void fbins(uint64_t n, char *buf) { // Lazy code 2.0, sorry
	float f = n;
	char *fs;
	if (f >= TB) {
		fs = "%.2f TiB";
		f /= TB;
	} else if (f >= GB) {
		fs = "%.2f GiB";
		f /= GB;
	} else if (f >= MB) {
		fs = "%.1f MiB";
		f /= MB;
	} else if (f >= KB) {
		fs = "%.1f KiB";
		f /= KB;
	} else
		fs = "%g B";
	snprintf(buf, 16, fs, f);
}

int sbinf(_vchar *input, uint64_t *size) {
	float f;
	char c;
#ifdef _WIN32
	if (swscanf(input, L"%f%c", &f, &c) != 2) {
		return 1;
	}
#else
	if (sscanf(input, "%f%c", &f, &c) != 2) {
		return 1;
	}
#endif
	if (f <= 0) return 2;
	uint64_t u;
	switch (c) {
	case 'T': case 't':
		u = f * 1099511627776; break;
	case 'G': case 'g':
		u = f * 1073741824; break;
	case 'M': case 'm':
		u = f * 1048576; break;
	case 'K': case 'k':
		u = f * 1024; break;
	case 'B': case 'b':
		u = f; break;
	default: return 3;
	}
	*size = u;
	return 0;
}

uint16_t bswap16(uint16_t s) {
	return s >> 8 | s << 8;
}

uint32_t bswap32(uint32_t s) {
	return	(s & 0x000000ff) << 24 | (s & 0x0000ff00) <<  8 |
		(s & 0x00ff0000) >>  8 | (s & 0xff000000) >> 24;
}

uint64_t bswap64(uint64_t s) {
	uint32_t *p = (uint32_t*)&s;
	return (uint64_t)bswap32(p[0]) << 32 | (uint64_t)bswap32(p[1]);
}

void print_a(char *p, uint8_t *a, size_t s) {
	size_t i = 0;
	printl(p);
	while (--s)
		printf(" %02X", a[i++]);
	putchar('\n');
}

int extcmp(_vchar *s1, const _vchar *s2) {
#ifdef _WIN32
	wchar_t *ext = wcsrchr(s1, '.');
	if (ext == NULL) // Not found
		return 0;
	++ext;
	if (*ext == 0) // Return if no characters after '.'
		return 0;
	return wcscmp(ext, s2) == 0;
#else
	const char *ext = strrchr(s1, '.');
	if (ext == NULL) // Not found
		return 0;
	++ext;
	if (*ext == 0) // Return if no characters after '.'
		return 0;
	return strcmp(ext, s2) == 0;
#endif
}

void printl(const char *s) {
	fputs(s, stdout);
}

int pow2(int n) {
	return (n & (n - 1)) == 0;
}

void wstra(char16 *src, char *dest, int dsize) {
	size_t bi = 0; // buffer index
	if (src[bi] == 0) {
		strcpy(dest, "<NO NAME>");
		return;
	}
	--dsize; // to include null byte later on
	while (bi < dsize && src[bi]) {
		if (src[bi] >= 0x20 && src[bi] <= 0x7E)
			dest[bi] = (char)src[bi];
		else
			dest[bi] = '?';
		++bi;
	}
	dest[bi] = 0;
}
