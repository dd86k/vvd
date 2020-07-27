#include <stdio.h>
#include <string.h>
#include "utils.h"

//
// fbins
//

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

//
// sbinf
//

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

//
// bswap16
//

uint16_t bswap16(uint16_t v) {
	return v >> 8 | v << 8;
}

//
// bswap32
//

uint32_t bswap32(uint32_t v) {
	v = (v >> 16) | (v << 16);
	return ((v & 0xFF00FF00) >> 8) | ((v & 0x00FF00FF) << 8);
}

//
// bswap64
//

uint64_t bswap64(uint64_t v) {
	v = (v >> 32) | (v << 32);
	v = ((v & 0xFFFF0000FFFF0000) >> 16) | ((v & 0x0000FFFF0000FFFF) << 16);
	return ((v & 0xFF00FF00FF00FF00) >> 8) | ((v & 0x00FF00FF00FF00FF) << 8);
}

//
// print_array
//

void print_array(char *p, uint8_t *a, size_t s) {
	size_t i = 0;
	putout(p);
	while (--s)
		printf(" %02X", a[i++]);
	putchar('\n');
}

//
// extcmp
//

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

//
// putout
//

void putout(const char *s) {
	fputs(s, stdout);
}

//
// pow2
//

int pow2(int n) {
	return (n & (n - 1)) == 0;
}

//
// wstra
//

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
