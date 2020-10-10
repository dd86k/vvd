#include <stdint.h>
#include "gpt.h"	// includes uid.h
#include "hash.h"

struct gpt_i_entry_t {
	uint32_t hash;
	const char *value;	// GUID string value
	const char *name;	// Partition type name
};

const struct gpt_i_entry_t gpt_i_types[] = {
	{ 0x86B2D89A, "00000000-0000-0000-0000-000000000000", "Empty" },
	{ 0x257B6BD0, "024DEE41-33E7-11D3-9D69-0008C781F39F", "MBR partition scheme" },
	{ 0x41248103, "E3C9E316-0B5C-4DB8-817D-F92DF00215AE", "Microsoft Reserved Partition" },
	{ 0, NULL, NULL }
};

const char* gpt_part_type_str(UID *uid) {
	UID_TEXT guid;
	uid_str(guid, uid, UID_GUID);

	uint32_t hash = hash_superfashhash_str(guid);

	const struct gpt_i_entry_t *entry = (struct gpt_i_entry_t*)gpt_i_types;
	while (entry->hash) {
		if (hash == entry->hash)
			return entry->name;
		++entry;
	}

	return NULL;
}
