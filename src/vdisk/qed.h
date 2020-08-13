/**
 * QED: QEMU Enhanced Disk
 * 
 * Little-endian format. Features clusters, including the header (cluster0).
 *
 * Layout:
 * +--------+----------+----------+----------+-----+
 * | Header | L1 table | Cluster0 | Cluster1 | ... |
 * +--------+----------+----------+----------+-----+
 *
 * For cluster allocation, there is a 2-level table:
 * 
 *            +----------+
 *            | L1 table |            <- Fixed
 *            +----------+
 *                 |
 *       +---------+---------+
 *       |         |         |
 * +----------+ +-----+ +----------+
 * | L2 table | | ... | | L2 table |  <- Allocated on-demand
 * +----------+ +-----+ +----------+
 *                 |
 *          +------+------+
 *          |      |      |
 *     +------+ +-----+ +------+
 *     | Data | | ... | | Data |      <- Cluster data
 *     +------+ +-----+ +------+
 *
 * Both the L1 table and L2 tables are of the same size (table_size * cluster_size).
 * The L1 table holds absolute file offsets to L2 table clusters, which the
 * L2 table holds absolute file offsets to data clusters.
 * 
 * https://wiki.qemu.org/Features/QED/Specification
 * https://github.com/qemu/qemu/blob/master/docs/interop/qed_spec.txt
 */

#include <stdint.h>

static const uint32_t QED_CLUSTER_DEFAULT	= 64 * 1024; // 64K
static const uint32_t QED_TABLE_DEFAULT	= 4; // 4 clusters
static const uint32_t QED_CLUSTER_MIN	= 4096; // 2^12, or 4 * 1024
static const uint32_t QED_CLUSTER_MAX	= 67108864; // 2^26, or 64 * 1024 * 1024
static const uint32_t QED_TABLE_MIN	= 1;
static const uint32_t QED_TABLE_MAX	= 16;

// Disk image uses a backup file for unallocated clusters
static const uint64_t QED_F_BACKING_FILE	= 1; // bit 0
// Disk image needs to be checked before use
static const uint64_t QED_F_NEED_CHECK	= 2; // bit 1
// Treat as raw
static const uint64_t QED_F_BACKING_FILE_NO_PROBE	= 4; // bit 2
// Known features
static const uint64_t QED_FEATS = QED_F_BACKING_FILE | QED_F_NEED_CHECK | QED_F_BACKING_FILE_NO_PROBE;

// QED header structure
typedef struct {
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
	// (QED_F_BACKING_FILE) Offset, in bytes from start of header, to the
	// name of the backing filename.
	uint32_t backup_name_offset;
	// (QED_F_BACKING_FILE) Size, in bytes, of the backing filename.
	uint32_t backup_name_size;
} QED_HDR;

typedef struct {
	uint64_t *offsets;	// L2 table offsets to data clusters
	uint64_t offset;	// Last loaded offset
	uint32_t tablesize;	// table_size
	uint32_t entries;	/// number of entries
} QED_L2CACHE;

struct VDISK;

int vdisk_qed_open(struct VDISK *vd, uint32_t flags, uint32_t internal);

int vdisk_qed_L2_load(struct VDISK *vd, uint64_t index);
