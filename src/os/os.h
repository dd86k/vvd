#include "../utils.h"
#include "../vdisk.h"

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
__OSFILE os_open(_vchar *path);
/**
 * Always create and overwrite a file path. This cannot create devices.
 */
__OSFILE os_create(_vchar *path);
/**
 * Seek into a position within the stream.
 */
int os_seek(__OSFILE handle, int64_t position, int flags);
/**
 * Read data from stream.
 */
int os_read(__OSFILE handle, void *buffer, size_t size);
/**
 * Write data to stream. Overwritting.
 */
int os_write(__OSFILE handle, void *buffer, size_t size);

/**
 * Print last OS error. Usually used within osutils.c and has no purpose outside
 * of it.
 */
void os_perror(const char *);