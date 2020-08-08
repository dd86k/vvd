/**
 * QED: QEMU Enhanced Disk
 * 
 * Little-endian format featuring clusters, including the header (cluster0).
 * 
 * https://wiki.qemu.org/Features/QED/Specification
 * https://github.com/qemu/qemu/blob/master/docs/interop/qed_spec.txt
 */

#include <stdint.h>

static const uint32_t QED_CLUSTER_DEFAULT	= 64 * 1024; // 64K
static const uint32_t QED_TABLE_DEFAULT	= 4; // 4 clusters
static const uint32_t QED_CLUSTER_MIN	= 4096; // 2^12
static const uint32_t QED_CLUSTER_MAX	= 67108864; // 2^26
static const uint32_t QED_TABLE_MIN	= 1;
static const uint32_t QED_TABLE_MAX	= 16;

// Disk image uses a backup file for unallocated clusters
static const uint64_t QED_FEAT_BACKING_FILE	= 1; // bit 0
// Disk image needs to be checked before use
static const uint64_t QED_FEAT_NEED_CHECK	= 2; // bit 1
// Treat as raw
static const uint64_t QED_FEAT_BACKING_FILE_NO_PROBE	= 4; // bit 2
// Known features
static const uint64_t QED_FEATS = QED_FEAT_BACKING_FILE | QED_FEAT_NEED_CHECK | QED_FEAT_BACKING_FILE_NO_PROBE;

// QED header structure
typedef struct QED_HDR {
	// Magic signature
	uint32_t magic;
	// In bytes, must be a power of 2 within [2^12, 2^26].
	uint32_t cluster_size;
	// For L1/L2 tables, in clusters, must be a power of 2 within [1, 16].
	uint32_t table_size;
	// In clusters
	uint32_t header_size;
	// Format feature flags
	uint64_t features;
	// Compat feature flags
	uint64_t compat_features;
	// Self-reset feature flags
	uint64_t autoclear_features;
	// L1 table offset in bytes
	uint64_t l1_offset;
	// Disk logical capacity in bytes
	uint64_t capacity;
	// (QED_FEAT_BACKING_FILE) Offset, in bytes from start of header, to the
	// name of the backing filename.
	uint32_t backup_name_offset;
	// (QED_FEAT_BACKING_FILE) Size, in bytes, of the backing filename.
	uint32_t backup_name_size;
} QED_HDR;

typedef struct VDISK VDISK;

int vdisk_qed_open(VDISK *vd, uint32_t flags, uint32_t internal);
