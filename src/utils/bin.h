#include <stdint.h>
#include <stdlib.h>

#ifdef _WIN32
// Represent a 'native' OS character
#define oschar wchar_t
#define osstr(quote) L##quote
#define OSCHARFMT "%ls"
#define oscmp wcscmp
#else // POSIX
// Represent a 'native' OS character
#define oschar char
#define osstr(quote) quote
#define OSCHARFMT "%s"
#define oscmp strcmp
#endif

#ifndef DEF_CHAR16
#define DEF_CHAR16
typedef uint16_t char16;
#endif

// Convert sector number/size to byte offset/size.
#define SECTOR_TO_BYTE(u) ((uint64_t)(u) << 9)

// Convert byte offset/size to sector number/size.
#define BYTE_TO_SECTOR(u) ((u) >> 9)

#define TiB 1099511627776
#define TB  1000000000000
#define GiB 1073741824
#define GB  1000000000
#define MiB 1048576
#define MB  1000000
#define KiB 1024
#define KB  1000

/// Binary character buffer length
#define BINSTR_LENGTH	16

/**
 * Get formatted binary (ISO) size with suffix, buffer fixed at 16 characters
 */
int bintostr(char *buffer, uint64_t size);

/**
 * Unformat a binary number into a 64-bit number.
 */
int strtobin(uint64_t *size, const oschar *input);

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
int extcmp(const oschar *s1, const oschar *s2);

/**
 * Checks if number is a power of 2.
 * 
 * Returns non-zero if number is a power of 2.
 */
uint32_t pow2(uint32_t);

/**
 * Checks if 64-bit number is a power of 2.
 * 
 * Returns non-zero if number is a power of 2.
 */
uint64_t pow264(uint64_t);

/**
 * Find nearest power of 2.
 */
uint32_t fpow2(uint32_t);

/**
 * Find nearest power of 2 with a 64-bit number.
 */
uint64_t fpow264(uint64_t);

/**
 * Make a string safer to print
 */
void str_s(char *str, int size);

/**
 * Convert an UTF-16 string to an ASCII string and returns number of charaters
 * copied with the destination's maximum buffer size. This function fills up
 * upto dsize-1 characters and inserts a null terminator and is useful when
 * processing GPT entries.
 * 
 * \returns Number of characters copied or negative on error
 */
int wstra(char *dest, char16 *src, int nchars);
