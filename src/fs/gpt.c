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

void gpt_info_stdout(GPT *gpt, uint32_t flags) {
	//TODO: Simplified view (and leave this one in "--raw-info")
	char gptsize[BINSTR_LENGTH];
	UID_TEXT diskguid;
	bintostr(gptsize, SECTOR_TO_BYTE(gpt->last.lba - gpt->first.lba));
	uid_str(diskguid, &gpt->guid, UID_GUID);
	printf(
	"\nGPT: v%u.%u (%u B), HDR CRC32 %08X, PT CRC32 %08X\n"
	"HEADER LBA %" PRIu64 ", BACKUP LBA %" PRIu64 "\n"
	"LBA %" PRIu64 " to %" PRIu64 " (%s)\n"
	"DISK GUID: %s\n"
	"PT LBA %" PRIu64 ", %u MAX ENTRIES, ENTRY SIZE %u\n",
	gpt->majorver, gpt->minorver, gpt->headersize, gpt->crc32, gpt->pt_crc32,
	gpt->current.lba, gpt->backup.lba,
	gpt->first.lba, gpt->last.lba, gptsize,
	diskguid,
	gpt->pt_location.lba, gpt->pt_entries, gpt->pt_esize
	);
}

//
// gpt_info_entries_stdout
//

void gpt_info_entries_stdout(VDISK *vd, GPT *gpt, uint64_t lba, uint32_t flags) {
	int max = gpt->pt_entries;	// maximum limiter, typically 128
	char partname[EFI_PART_NAME_LENGTH];
	char partsize[BINSTR_LENGTH];
	UID_TEXT partguid, typeguid;
	GPT_ENTRY entry; // GPT entry

START:
	if (vdisk_read_sector(vd, &entry, lba)) {
		fputs("gpt_info_entries_stdout: Could not read GPT_ENTRY", stderr);
		return;
	}

	if (uid_nil(&entry.type))
		return;

	uid_str(typeguid, &entry.type, UID_GUID);
	uid_str(partguid, &entry.part, UID_GUID);
	wstra(partname, entry.partname, EFI_PART_NAME_LENGTH);

	bintostr(partsize, SECTOR_TO_BYTE(entry.last.lba - entry.first.lba));
	printf(
	"%" PRIu64 ". %-36s\n"
	"  LBA %" PRIu64 " TO %" PRIu64 " (%s)\n"
	"  PART GUID: %s\n"
	"  TYPE GUID: %s\n"
	"  FLAGS: %XH, PART FLAGS: %XH\n",
	lba - 1, partname,
	entry.first.lba, entry.last.lba, partsize,
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
