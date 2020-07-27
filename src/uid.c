#include <stdio.h>
#include <assert.h>
#include "guid.h"
#include "utils.h"

// __GUID to 36-character buffer string
int guid_tostr(char *buffer, __GUID *guid) {
	return snprintf(buffer, GUID_TEXT_SIZE,
	"%08X-%04X-%04X-%04X-%02X%02X%02X%02X%02X%02X",
	guid->time_low, guid->time_mid, guid->time_ver, guid->clock,
	guid->data[10], guid->data[11], guid->data[12],
	guid->data[13], guid->data[14], guid->data[15]
	);
}

// __GUID to 36-character buffer string
int uuid_tostr(char *buffer, __GUID *guid) {
	return snprintf(buffer, GUID_TEXT_SIZE,
	"%08X-%04X-%04X-%04X-%02X%02X%02X%02X%02X%02X",
	bswap32(guid->time_low), bswap16(guid->time_mid),
	bswap16(guid->time_ver), bswap16(guid->clock),
	guid->data[10], guid->data[11], guid->data[12],
	guid->data[13], guid->data[14], guid->data[15]
	);
}

// 36-character string to __GUID, version 4a
int guid_frstr(__GUID *guid, char *buffer) {
	assert(0);
	return 0;
}

void guid_swap(__GUID *guid) {
	guid->time_low = bswap32(guid->time_low);
	guid->time_mid = bswap32(guid->time_mid);
	guid->time_ver = bswap32(guid->time_ver);
	guid->clock = bswap32(guid->clock);
}

// "guid_is_not_empty" is an ugly name
int guid_nil(__GUID *guid) {
#if __SIZE_WIDTH__ == 64
	if (guid->u64[0]) return 0;
	if (guid->u64[1]) return 0;
#else
	if (guid->u32[0]) return 0;
	if (guid->u32[1]) return 0;
	if (guid->u32[2]) return 0;
	if (guid->u32[3]) return 0;
#endif
	return 1;
}

int guid_cmp(__GUID *guid1, __GUID *guid2) {
	assert(0);
	return 0;
}
