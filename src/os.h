#include "utils.h"
#include "vdisk.h"

#ifdef _WIN32	// Windows
#include <windows.h>
typedef HANDLE __OSFILE;

#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#ifndef __OSFILE_H
#define __OSFILE_H
typedef int __OSFILE;
#endif // __OSFILE_H
#endif

/**
 * Open a file stream. File or device must exist. Consult documentation on
 * which devices are supported.
 */
__OSFILE os_open(const _oschar *path);
/**
 * Always create and overwrite a file path. This cannot create devices.
 */
__OSFILE os_create(const _oschar *path);
/**
 * Seek into a position within the stream.
 */
int os_fseek(__OSFILE fd, int64_t position, int flags);
/**
 * Read data from stream from current position.
 */
int os_fread(__OSFILE fd, void *buffer, size_t size);
/**
 * Write data to stream at current position, overwrites.
 */
int os_fwrite(__OSFILE fd, void *buffer, size_t size);
/**
 * Get the file size, or the disk size, in bytes. If the handle is a file,
 * the file size is set, otherwise if the handle is a block device, the
 * disk size is set. Uses GetFileSizeEx (Windows) or stat/ioctl (Linux).
 */
int os_fsize(__OSFILE fd, uint64_t *size);
/**
 * Zero write to file.
 */
int os_falloc(__OSFILE fd, uint64_t fsize);
