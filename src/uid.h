#pragma once

#include <stdint.h>

enum {
	UID_GUID	= 0,
	UID_UUID	= 1,
	UID_LENGTH	= 38	// usually 36 but.. {} and \0
};
typedef char UID_TEXT[UID_LENGTH];
/**
 * UUID/GUID structure
 */
typedef struct UID {
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
} UID;

/**
 * Format a UID (GUID/UUID) to a string buffer.
 */
int uid_str(UID *uid, char *str, int target);
/**
 * Byte swap GUID/UUID fields to convert a GUID into an UUID or vice-versa.
 * Useful when the endianess differs from a machine. GUIDs is usually
 * little-endian and UUIDs are usually big-endian.
 * 
 * \param id UID structure
 */
void uid_swap(UID *uid);
/**
 * Verifies if a GUID/UUID is nil (null, empty).
 */
int uid_nil(UID *uid);
/**
 * Compares two GUIDs/UUIDs.
 */
int uid_cmp(UID *uid1, UID *uid2);
