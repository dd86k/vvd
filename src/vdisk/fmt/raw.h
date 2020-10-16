struct VDISK;

int vdisk_raw_open(struct VDISK *vd, uint32_t flags, uint32_t internal);
int vdisk_raw_read_lba(struct VDISK *vd, void *buffer, uint64_t index);