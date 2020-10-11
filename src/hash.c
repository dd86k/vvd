#include <stdint.h>
#include <string.h>
#include <assert.h>
#include "hash.h"

static const uint32_t PRIME32_1 = 2654435761U;
static const uint32_t PRIME32_2 = 2246822519U;
static const uint32_t PRIME32_3 = 3266489917U;
static const uint32_t PRIME32_4 = 668265263U;
static const uint32_t PRIME32_5 = 374761393U;

// Internal: rotate 32-bit value (v) with offset (o)
uint32_t hash_rot(uint32_t v, int o) {
	return (v << o) | (v >> (32 - o));
}

//
// hash_string
//

uint32_t hash_string(const char *data) {
	return hash_compute_s(data, strlen(data), 0);
}

//
// hash_compute
//

uint32_t hash_compute(const char *data, uint32_t length) {
	return hash_compute_s(data, length, 0);
}

//
// hash_compute_s
//

uint32_t hash_compute_s(const char *data, uint32_t length, uint32_t seed) {
	union {
		uint32_t *u32;
		uint8_t  *u8;
		const char *c8;
	} p;
	uint32_t hash;
	const char *end = data + length;	// End of data pointer

	p.c8 = data;

	//
	// Main loop
	//

	if (length >= 16) {
		const char *limit = end - 16;

		uint32_t v1 = seed + PRIME32_1 + PRIME32_2;
		uint32_t v2 = seed + PRIME32_2;
		uint32_t v3 = seed + 0;
		uint32_t v4 = seed - PRIME32_1;

		// Main loop proccesses 4*4 Bytes of information per iteration
		do {
			v1 += p.u32[0] * PRIME32_2;
			v1 =  hash_rot(v1, 13);
			v1 *= PRIME32_1;

			v2 += p.u32[1] * PRIME32_2;
			v2 =  hash_rot(v2, 13);
			v2 *= PRIME32_1;

			v3 += p.u32[2] * PRIME32_2;
			v3 =  hash_rot(v3, 13);
			v3 *= PRIME32_1;

			v4 += p.u32[3] * PRIME32_2;
			v4 =  hash_rot(v4, 13);
			v4 *= PRIME32_1;

			p.u32 += 4;
		} while (p.c8 <= limit);

		hash =	hash_rot(v1, 1)  + hash_rot(v2, 7) +
			hash_rot(v3, 12) + hash_rot(v4, 18);
	} else {
		hash = seed + PRIME32_5;
	}

	hash += length;

	//
	// Finalization
	//

	// Per 4 Bytes
	while (p.c8 <= end - 4) {
		hash += *p.u32 * PRIME32_3;
		hash =  hash_rot(hash, 17) * PRIME32_4;
		++p.u32;
	}

	// Per Byte
	while (p.c8 < end) {
		hash += *p.u8 * PRIME32_5;
		hash =  hash_rot(hash, 11) * PRIME32_1;
		++p.u8;
	}

	//
	// Avalanche
	//

	hash ^= hash >> 15;
	hash *= PRIME32_2;
	hash ^= hash >> 13;
	hash *= PRIME32_3;
	hash ^= hash >> 16;

	return hash;
}
