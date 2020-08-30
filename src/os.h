#include "utils.h"
#include "vdisk.h"

#ifdef _WIN32
#include <Windows.h>
typedef HANDLE __OSFILE;
#else // Posix
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

//
// File functions
//

/**
 * Open a file stream. File or device must exist. Consult documentation on
 * which devices are supported.
 */
__OSFILE os_fopen(const oschar *path);

/**
 * Always create and overwrite a file path. This cannot create devices.
 */
__OSFILE os_fcreate(const oschar *path);

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

//
// Progress functions
//

#ifndef DEFINITION_OS_PROGRESS
#define DEFINITION_OS_PROGRESS
enum {
	PROG_MODE_NONE	= 0,
	PROG_MODE_CUR_MAX	= 1,
	PROG_MODE_CUR_ONLY	= 2,
	PROG_MODE_POURCENT	= 3,
	//TODO: PROG_MODE_INDETERMINATE

	PROG_FLAG_REMOVE	= 0x100, // Remove progress bar when done
};

struct progress_t {
	uint32_t current, maximum;
	uint16_t initx, inity, lenx, leny;
	char *bfill;	// fill char buffer
	char *bspace;	// empty char buffer
	uint32_t flags;
#if _WIN32
	HANDLE fd;
#else
	size_t res1;
#endif
};
#endif // DEFINITION_OS_PROGRESS

/**
 * Initiate a new progress bar. This function allocates, but manages its own 
 * memory.
 * 
 * \param p struct progress_t pointer
 * \param flags See PROG enumerations
 * \param max Maximum value
 * 
 * \returns Status code
 */
int os_pinit(struct progress_t *p, uint32_t flags, uint32_t max);

/**
 * Update the progress bar with a new current value.
 * 
 * \param p struct progress_t pointer
 * \param val New current value
 * 
 * \returns Status code
 */
int os_pupdate(struct progress_t *p, uint32_t val);

/**
 * Indiate that the progress bar is finished and will no longer be used. This
 * function frees memory it had previously allocated.
 * 
 * \param p struct progress_t pointer
 * 
 * \returns Status code
 */
int os_pfinish(struct progress_t *p);
