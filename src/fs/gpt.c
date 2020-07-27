#include <stdio.h>
#include <stdint.h>
#include <string.h>	// strcpy
#include <inttypes.h>
#include "gpt.h"	// includes guid.h
#include "../utils.h"
#include "../vdisk.h"

//
// gpt_info
//

void gpt_info(GPT *gpt) {
	GUID_TEXT diskguid;
	guid_tostr(diskguid, &gpt->guid);
	printf(
	"\n* GPT v%u.%u (%u B), HDR CRC32 %08X, PT CRC32 %08X\n"
	"MAIN LBA %u, BACKUP LBA %u, FIRST LBA %u, LAST LBA %u\n"
	"PT LBA %u, %u MAX ENTRIES, ENTRY SIZE %u\n"
	"DISK GUID: %s\n",
	gpt->majorv, gpt->minorv, gpt->headersize, gpt->crc32, gpt->pt_crc32,
	gpt->current.low, gpt->backup.low, gpt->firstlba.low, gpt->lastlba.low,
	gpt->pt_location.low, gpt->pt_entries, gpt->pt_esize,
	diskguid
	);
}

//
// gpt_list_pe_vd
//

void gpt_list_pe_vd(VDISK *vd, GPT *gpt) {
	int max = gpt->pt_entries;	// maximum limiter
	char partname[EFI_PART_NAME_LENGTH];
	GUID_TEXT partguid, typeguid;
	GPT_ENTRY entry;
	uint32_t lba = 2;
	//TODO: Consider reading a few entries per loop to avoid reading too often

START:
	//if (os_read(vd->fd, &entry, sizeof(GPT_ENTRY))) {
	if (vdisk_read_lba(vd, &entry, lba)) {
		fputs("gpt_list_pe_vd: Could not read GPT_ENTRY", stderr);
		return;
	}

	if (guid_nil(&entry.type))
		return;

	guid_tostr(typeguid, &entry.type);
	guid_tostr(partguid, &entry.part);
	wstra(entry.partname, partname, EFI_PART_NAME_LENGTH);

	printf(
		"%u. %-36s\n"
		"	LBA %" PRIu64 " TO %" PRIu64 "\n"
		"	PART: %s\n"
		"	TYPE: %s\n"
		"	FLAGS: %XH, PART FLAGS: %XH\n",
		lba - 1, partname,
		entry.firstlba.lba, entry.lastlba.lba,
		partguid, typeguid,
		entry.flags, entry.flags_part
	);
	// GPT flags
	if (entry.flags & EFI_PE_PLATFORM_REQUIRED)
		puts("+ Platform required");
	if (entry.flags & EFI_PE_PLATFORM_REQUIRED)
		puts("+ Firmware ignore");
	if (entry.flags & EFI_PE_PLATFORM_REQUIRED)
		puts("+ Legacy BIOS bootable");
	// Partition flags
	if (entry.flags_part & EFI_PE_SUCCESSFUL_BOOT)
		puts("+ (Google) Successful boot");
	if (entry.flags_part & EFI_PE_READ_ONLY)
		puts("+ (Microsoft) Read-only");
	if (entry.flags_part & EFI_PE_SHADOW_COPY)
		puts("+ (Microsoft) Shadow copy");
	if (entry.flags_part & EFI_PE_HIDDEN)
		puts("+ (Microsoft) Hidden");

	if (max <= 0)
		return;
	++lba; --max;
	goto START;
}
