#include <stdio.h>
#include <stdint.h>
#include <string.h>	// strcpy
#include <inttypes.h>
#include "gpt.h"	// includes uid.h
#include "../utils.h"
#include "../vdisk.h"

//
// gpt_info_stdout
//

void gpt_info_stdout(GPT *gpt) {
	UID_TEXT diskguid;
	uid_str(&gpt->guid, diskguid, UID_GUID);
	printf(
	"\nGPT: v%u.%u (%u B), HDR CRC32 %08X, PT CRC32 %08X\n"
	"MAIN LBA %u, BACKUP LBA %u, FIRST LBA %u, LAST LBA %u\n"
	"PT LBA %u, %u MAX ENTRIES, ENTRY SIZE %u\n"
	"DISK GUID: %s\n",
	gpt->majorver, gpt->minorver, gpt->headersize, gpt->crc32, gpt->pt_crc32,
	gpt->current.low, gpt->backup.low, gpt->firstlba.low, gpt->lastlba.low,
	gpt->pt_location.low, gpt->pt_entries, gpt->pt_esize,
	diskguid
	);
}

//
// gpt_info_entries_stdout
//

void gpt_info_entries_stdout(VDISK *vd, GPT *gpt, uint64_t lba) {
	int max = gpt->pt_entries;	// maximum limiter
	char partname[EFI_PART_NAME_LENGTH];
	UID_TEXT partguid, typeguid;
	GPT_ENTRY entry;

START:
	if (vdisk_read_sector(vd, &entry, lba)) {
		fputs("gpt_info_entries_stdout: Could not read GPT_ENTRY", stderr);
		return;
	}

	if (uid_nil(&entry.type))
		return;

	uid_str(&entry.type, typeguid, UID_GUID);
	uid_str(&entry.part, partguid, UID_GUID);
	wstra(entry.partname, partname, EFI_PART_NAME_LENGTH);

	printf(
		"%" PRIu64 ". %-36s\n"
		"  LBA %" PRIu64 " TO %" PRIu64 "\n"
		"  PART: %s\n"
		"  TYPE: %s\n"
		"  FLAGS: %XH, PART FLAGS: %XH\n",
		lba - 1, partname,
		entry.firstlba.lba, entry.lastlba.lba,
		partguid, typeguid,
		entry.flags, entry.partflags
	);

	// GPT flags
	if (entry.flags & EFI_PE_PLATFORM_REQUIRED)
		puts("+ Platform required");
	if (entry.flags & EFI_PE_PLATFORM_REQUIRED)
		puts("+ Firmware ignore");
	if (entry.flags & EFI_PE_PLATFORM_REQUIRED)
		puts("+ Legacy BIOS bootable");

	// Partition flags
	if (entry.partflags & EFI_PE_SUCCESSFUL_BOOT)
		puts("+ (Google) Successful boot");
	if (entry.partflags & EFI_PE_READ_ONLY)
		puts("+ (Microsoft) Read-only");
	if (entry.partflags & EFI_PE_SHADOW_COPY)
		puts("+ (Microsoft) Shadow copy");
	if (entry.partflags & EFI_PE_HIDDEN)
		puts("+ (Microsoft) Hidden");

	if (max <= 0)
		return;
	++lba; --max;
	goto START;
}
