/**
 * Hashing module re-written for clarity.
 * 
 * Original source: http://www.azillionmonkeys.com/qed/hash.html
 */


/**
 * Hash a string
 */
uint32_t hash_superfashhash_str(const char *data);

/**
 * Hash data
 */
uint32_t hash_superfashhash(const char *data, uint32_t len);

/**
 * Continue the computation of the hash with the previous result
 */
uint32_t hash_superfashhash_compute(const char *data, uint32_t len, uint32_t seed);