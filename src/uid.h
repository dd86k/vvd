#pragma once

#include <stdint.h>

enum {
	UID_ASIS	= 0,	// Leave as-is, no swap intended
	UID_GUID	= 1,	// Meant as GUID, swap if on big-endian
	UID_UUID	= 2,	// Meant as UUID, swap if on little-endian
	UID_BUFFER_LENGTH	= 40	// usually 36 but.. {} and \0
};
// Text buffer
typedef char UID_TEXT[UID_BUFFER_LENGTH];

/**
 * UUID/GUID structure
 */
typedef struct UID {
	union {
		uint8_t  data[16];
		uint16_t u16[8];
		uint32_t u32[4];
		uint64_t u64[2]; // Preferred when __SIZE_WIDTH__ == 64
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
 * 
 * \param str String buffer target
 * \param uid UID structure source
 * \param type GUID, UUID, or ASIS
 * \returns The result of snprintf
 */
int uid_str(char *str, UID *uid, int type);
/**
 * Parse a string GUID or UUID into a UID structure.
 * 
 * Uses sscanf for parsing.
 * 
 * \param uid UID structure target
 * \param str String buffer source
 * \param type GUID, UUID, or ASIS
 * \returns 0 when successful, >0 on error, and <0 on sscanf error
 */
int uid_parse(UID *uid, const char *str, int type);
/**
 * Byte swap GUID/UUID fields to convert a GUID into an UUID or vice-versa.
 * Useful when the endianess differs from a machine. GUIDs is usually
 * little-endian and UUIDs are usually big-endian.
 * 
 * \param uid UID structure
 */
void uid_swap(UID *uid);
/**
 * Verifies if a GUID/UUID is nil (null, empty).
 * 
 * \param uid UID structure
 * \returns Non-zero if nil
 */
int uid_nil(UID *uid);
/**
 * Compares two GUIDs/UUIDs.
 */
int uid_cmp(UID *uid1, UID *uid2);
