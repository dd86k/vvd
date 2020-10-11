#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "uid.h"
#include "utils.h"
#include "platform.h"

//TODO: uid_create(UID*,int): Create UID with target in mind

//
// uid_str
//

int uid_str(char *buf, UID *uid, int type) {
	#ifdef ENDIAN_LITTLE
		if (type == UID_UUID)
			uid_swap(uid);
	#else
		if (type == UID_GUID)
			uid_swap(uid);
	#endif
	return snprintf(buf, UID_BUFFER_LENGTH,
		"%08X-%04X-%04X-%04X-%02X%02X%02X%02X%02X%02X",
		uid->time_low, uid->time_mid, uid->time_ver,
		bswap16(uid->clock),
		uid->data[10], uid->data[11], uid->data[12],
		uid->data[13], uid->data[14], uid->data[15]);
}

//
// uid_parse
//

int uid_parse(UID *uid, const char *buf, int type) {
	unsigned int time_low, time_mid, time_ver, clock,
		data10, data11, data12, data13, data14, data15;
	int r = sscanf(buf, "%08X-%04X-%04X-%04X-%02X%02X%02X%02X%02X%02X",
		&time_low, &time_mid, &time_ver, &clock,
		&data10, &data11, &data12,
		&data13, &data14, &data15);
	if (r < 0 || r != 10)
		return r;
	uid->time_low = time_low;
	uid->time_mid = (uint16_t)time_mid;
	uid->time_ver = (uint16_t)time_ver;
	uid->clock    = bswap16(clock);
	uid->data[10] = (uint8_t)data10;
	uid->data[11] = (uint8_t)data11;
	uid->data[12] = (uint8_t)data12;
	uid->data[13] = (uint8_t)data13;
	uid->data[14] = (uint8_t)data14;
	uid->data[15] = (uint8_t)data15;
	return 0;
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
	if (uid->u64[0] || uid->u64[1])
		return 0;
#else
	if (uid->u32[0] || uid->u32[1] || uid->u32[2] || uid->u32[3])
		return 0;
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
