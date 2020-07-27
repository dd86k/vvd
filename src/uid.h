#pragma once

#include <stdint.h>

#define GUID_TEXT_SIZE	38	// usually 36 but.. {} and \0
typedef char GUID_TEXT[GUID_TEXT_SIZE];
typedef struct __GUID {	// GUID/UUID structure
	union {
		uint8_t  data[16];
		uint16_t u16[8];
		uint32_t u32[4];
		uint64_t u64[2]; // Preferred to use when size width = 64
		struct {
			uint32_t time_low;
			uint16_t time_mid;
			uint16_t time_ver;	// and time_hi
			uint16_t clock;	// seq_hi and res_clock_low
			uint8_t  node[6];
		};
	};
} __GUID;

/**
 * Format a GUID to a string buffer.
 */
int guid_tostr(char *, __GUID *);
/**
 * Format a UUID to a string buffer.
 */
int uuid_tostr(char *, __GUID *);
/**
 * Convert a GUID string from string.
 */
int guid_frstr(__GUID *, char *);
/**
 * Byte swap GUID/UUID fields to convert a GUID into an UUID or vice-versa.
 * Useful when the endianess differs from a machine. GUIDs is usually
 * little-endian and UUIDs are usually big-endian.
 */
void guid_swap(__GUID *guid);
/**
 * Verifies if a GUID/UUID is nil (null, empty).
 */
int guid_nil(__GUID *);
/**
 * Compares two GUIDs or two UUIDs.
 */
int guid_cmp(__GUID *, __GUID *);
