#include <stdio.h>
#include <assert.h>
#include "uid.h"
#include "utils.h"
#include "platform.h"

//TODO: uid_create(UID*,int): Create UID with target in mind

//
// uid_str
//

int uid_str(char *buf, UID *uid, int target) {
	#ifdef ENDIAN_LITTLE
		if (target == UID_UUID)
			uid_swap(uid);
	#else
		if (target == UID_GUID)
			uid_swap(uid);
	#endif
	return snprintf(buf, UID_BUFFER_LENGTH,
	"%08X-%04X-%04X-%04X-%02X%02X%02X%02X%02X%02X",
	uid->time_low, uid->time_mid, uid->time_ver, uid->clock,
	uid->data[10], uid->data[11], uid->data[12],
	uid->data[13], uid->data[14], uid->data[15]
	);
}

//
// uid_swap
//

void uid_swap(UID *uid) {
	uid->time_low = bswap32(uid->time_low);
	uid->time_mid = bswap16(uid->time_mid);
	uid->time_ver = bswap16(uid->time_ver);
	uid->clock = bswap16(uid->clock);
}

//
// uid_nil
//

int uid_nil(UID *uid) {
#if __SIZE_WIDTH__ == 64
	if (uid->u64[0]) return 0;
	if (uid->u64[1]) return 0;
#else
	if (uid->u32[0]) return 0;
	if (uid->u32[1]) return 0;
	if (uid->u32[2]) return 0;
	if (uid->u32[3]) return 0;
#endif
	return 1;
}

//
// uid_cmp
//

int uid_cmp(UID *uid1, UID *uid2) {
	assert(0);
	return 0;
}
