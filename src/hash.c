#include <stdint.h>
#include <string.h>
#include "hash.h"

//
// hash_superfashhash_str
//

uint32_t hash_superfashhash_str(const char *data) {
	uint32_t l = (uint32_t)strlen(data);
	return hash_superfashhash_compute(data, l, l);
}

//
// hash_superfashhash
//

uint32_t hash_superfashhash(const char *data, uint32_t len) {
	return hash_superfashhash_compute(data, len, len);
}

//
// hash_superfashhash_compute
//

uint32_t hash_superfashhash_compute(const char *data, uint32_t len, uint32_t seed) {
	const uint16_t *du16 = (const uint16_t*)data;	// u16 data pointer
	uint32_t hash = seed, temp;
	int rem;	// Remaining bits

	if (len <= 0 || data == NULL) return 0;

	rem = len & 3;
	len >>= 2;	// len /= 4

	// The main loop proccesses 4 Bytes of information every iteration
	while (len > 0) {
		hash += *du16;
		temp = (du16[1] << 11) ^ hash;
		hash = (hash << 16) ^ temp;
		data += 2;
		hash += hash >> 11;
		--len;
	}

	/* Handle end cases */
	switch (rem) {
	case 3:
		hash += *du16;
		hash ^= hash << 16;
		hash ^= ((signed char)du16[1]) << 18;	// sizeof(uint16_t)
		hash += hash >> 11;
		break;
	case 2:
		hash += *du16;
		hash ^= hash << 11;
		hash += hash >> 17;
		break;
	case 1:
		hash += *(signed char*)du16;
		hash ^= hash << 10;
		hash += hash >> 1;
	}

	// Force "avalanching" of final 127 bits
	hash ^= hash << 3;
	hash += hash >> 5;
	hash ^= hash << 4;
	hash += hash >> 17;
	hash ^= hash << 25;
	hash += hash >> 6;

	return hash;
}