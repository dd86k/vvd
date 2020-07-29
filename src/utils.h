#include <stdint.h>
#include <stdlib.h>

#ifdef _WIN32
#define _vchar wchar_t
#else // POSIX
#define _vchar char
#endif

#ifndef _CHAR16
#define _CHAR16
typedef uint16_t char16;
#endif

// Convert sector number/size to byte offset/size.
#define SECTOR_TO_BYTE(u) ((uint64_t)(u) << 9)

// Convert byte offset/size to sector number/size.
#define BYTE_TO_SECTOR(u) ((u) >> 9)

#define TB 1099511627776
#define GB 1073741824
#define MB 1048576
#define KB 1024

/**
 * Function alias of fputs(*, stdout) to avoid argument bloat.
 */
void putout(const char *s);

/// Binary character buffer length
#define BIN_FLENGTH	16

/**
 * Get formatted binary (ISO) size with suffix, buffer fixed at 16 characters
 */
void fbins(uint64_t, char *);
/**
 * 
 */
int sbinf(const _vchar *input, uint64_t *size);

/**
 * Print array with prefix string
 */
void print_array(char *p, uint8_t *a, size_t s);

/**
 * Byte swap a 16-bit (2-Byte) value.
 */
uint16_t bswap16(uint16_t);
/**
 * Byte swap a 32-bit (4-Byte) value.
 */
uint32_t bswap32(uint32_t);
/**
 * Byte swap a 64-bit (8-Byte) value.
 */
uint64_t bswap64(uint64_t);

#ifdef _WIN32
	#define EXT_VDI L"vdi"
	#define EXT_VMDK L"vmdk"
	#define EXT_VHD L"vhd"
#else
	#define EXT_VDI "vdi"
	#define EXT_VMDK "vmdk"
	#define EXT_VHD "vhd"
#endif

/**
 * Compare file path with constant extension string.
 * 
 * E.g. `extcmp("test.bin", "bin")` evaluates to non-zero
 */
int extcmp(const _vchar *s1, const _vchar *s2);

/**
 * Checks if number is a power of 2.
 * 
 * Returns non-zero if number is a power of 2.
 */
int pow2(int);

/**
 * Convert an UTF-16 string to an ASCII string and returns number of charaters
 * copied with the destination's maximum buffer size. This function fills up
 * upto dsize-1 characters and inserts a null terminator and is useful when
 * processing GPT entries.
 */
void wstra(char16 *src, char *dest, int dsize);
