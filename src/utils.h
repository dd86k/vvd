#include <stdint.h>
#include <stdlib.h>

#ifdef _WIN32
// Represent a 'native' OS character
#define _oschar wchar_t
#else // POSIX
// Represent a 'native' OS character
#define _oschar char
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
 * Imitates `fputs(*, stdout)` for ease of typing.
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
int sbinf(const _oschar *input, uint64_t *size);

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

/**
 * Compare file path with constant extension string. This is currently only
 * used in the 'new' operation to obtain what disk type to create.
 * 
 * E.g. `extcmp("test.bin", "bin")` evaluates to non-zero
 */
int extcmp(const _oschar *s1, const _oschar *s2);

/**
 * Checks if number is a power of 2.
 * 
 * Returns non-zero if number is a power of 2.
 */
int pow2(int);

/**
 * Find nearest power of 2.
 */
unsigned int fpow2(unsigned int);

/**
 * Convert an UTF-16 string to an ASCII string and returns number of charaters
 * copied with the destination's maximum buffer size. This function fills up
 * upto dsize-1 characters and inserts a null terminator and is useful when
 * processing GPT entries.
 */
void wstra(char16 *src, char *dest, int dsize);
