#include <stdio.h>
#include <inttypes.h> // PRIxxx
#include "../vdisk.h"
#include "../fs/mbr.h"
#include "../utils.h"

void vhd_info(VDISK *vd) { // big-endian
	char sizec[BIN_FLENGTH], sizeo[BIN_FLENGTH], uuid[GUID_TEXT_SIZE];
	char *type, *os;

	switch (vd->vhd.type) {
	case VHD_DISK_FIXED:	type = "fixed"; break;
	case VHD_DISK_DYN:	type = "dynamic"; break;
	case VHD_DISK_DIFF:	type = "differencing"; break;
	default:
		type = vd->vhd.type <= 6 ? "reserved (deprecated)" : "unknown";
	}

	switch (vd->vhd.creator_os) {
	case VHD_OS_WIN:	os = "Windows"; break;
	case VHD_OS_MAC:	os = "macOS"; break;
	default:	os = "unknown"; break;
	}

	uuid_tostr(uuid, &vd->vhd.uuid);
	fbins(vd->vhd.size_current, sizec);
	fbins(vd->vhd.size_original, sizeo);
	printf(
		"Conectix/Microsoft VHD vdisk v%u.%u, %s %s/%s, %.4s v%u.%u on %s\n"
		"Cylinders: %u, Heads: %u, Sectors: %u\n"
		"CRC32: %08X, UUID: %s\n"
		,
		vd->vhd.major, vd->vhd.minor, type, sizec, sizeo,
		vd->vhd.creator_app, vd->vhd.creator_major, vd->vhd.creator_minor, os,
		vd->vhd.cylinders, vd->vhd.heads, vd->vhd.sectors,
		vd->vhd.checksum,
		uuid
	);
	if (vd->vhd.type != VHD_DISK_FIXED) {
		char paruuid[GUID_TEXT_SIZE];
		uuid_tostr(paruuid, &vd->vhddyn.parent_uuid);
		printf(
			"Dynamic header v%u.%u, data: %" PRIu64 ", table: %" PRIu64 "\n"
			"Blocksize: %u, checksum: %08X\n"
			"Parent UUID: %s, Parent timestamp: %u\n"
			"%u BAT Entries, %u maximum BAT entries\n"
			,
			vd->vhddyn.minor, vd->vhddyn.major,
			vd->vhddyn.data_offset, vd->vhddyn.table_offset,
			vd->vhddyn.blocksize, vd->vhddyn.checksum,
			paruuid, vd->vhddyn.parent_timestamp,
			vd->u32nblocks, vd->vhddyn.max_entries
		);
	}
	if (vd->vhd.savedState)
		puts("+ Saved state");
}