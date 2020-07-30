#include <stdio.h>
#include "os.h"
#ifndef _WIN32
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/fs.h>
#endif

__OSFILE os_open(const _oschar *path) {
#ifdef _WIN32
	__OSFILE fd = CreateFileW(
		path,
		GENERIC_READ | GENERIC_WRITE,	// dwDesiredAccess
		0,	// dwShareMode: No sharing
		NULL,	// lpSecurityAttributes
		OPEN_EXISTING,	// dwCreationDisposition: Open only if existing
		0,	// dwFlagsAndAttributes
		NULL	// hTemplateFile
	);
	if (fd == INVALID_HANDLE_VALUE)
		return 0;
#else
	__OSFILE fd = open(path, O_RDWR);
	if (fd == -1)
		return 0;
#endif
	return fd;
}

__OSFILE os_create(const _oschar *path) {
#ifdef _WIN32
	__OSFILE fd = CreateFileW(
		path,
		GENERIC_READ | GENERIC_WRITE,	// dwDesiredAccess
		0,	// dwShareMode: No sharing
		NULL,	// lpSecurityAttributes
		CREATE_ALWAYS,	// dwCreationDisposition: Always create
		0,	// dwFlagsAndAttributes
		NULL	// hTemplateFile
	);
	if (fd == INVALID_HANDLE_VALUE)
		return 0;
#else
	__OSFILE fd = open(path, O_RDWR | O_CREAT | O_TRUNC);
	if (fd == -1)
		return 0;
#endif
	return fd;
}

int os_seek(__OSFILE handle, int64_t pos, int flags) {
#ifdef _WIN32
	LARGE_INTEGER a;
	a.QuadPart = pos;
	if (SetFilePointerEx(handle, a, NULL, flags) == 0)
		return -1;
#else
	if (lseek(handle, (off_t)pos, flags) == -1)
		return -1;
#endif
	return 0;
}

int os_read(__OSFILE handle, void *buffer, size_t size) {
#ifdef _WIN32
	DWORD r;
	if (ReadFile(handle, buffer, size, &r, NULL) == 0)
		return -1;
	/*if (r != size) {
		fprintf(stderr, "os_read: Failed to read %u/%u bytes",
			(uint32_t)r, (uint32_t)size);
		return -2;
	}*/
#else
	ssize_t r;
	if ((r = read(handle, buffer, size)) == -1)
		return -1;
	/*if (r != size) {
		fprintf(stderr, "os_read: Failed to read %d/%u bytes",
			(int32_t)r, (uint32_t)size);
		return -2;
	}*/
#endif
	return 0;
}

int os_write(__OSFILE handle, void *buffer, size_t size) {
#ifdef _WIN32
	DWORD r;
	if (WriteFile(handle, buffer, size, &r, NULL) == 0)
		return -1;
	/*if (r != size) {
		fprintf(stderr, "os_write: Failed to write %u bytes", (unsigned int)r);
		return -2;
	}*/
#else
	ssize_t r;
	if ((r = write(handle, buffer, size)) == -1)
		return -1;
	/*if (r != size) {
		fprintf(stderr, "os_write: Failed to read %u bytes", (unsigned int)r);
		return -2;
	}*/
#endif
	return 0;
}

//
// os_size
//

int os_size(__OSFILE handle, uint64_t *size) {
#if _WIN32
	LARGE_INTEGER li;
	if (GetFileSizeEx(handle, &li) == 0)
		return 1;
	*size = li.QuadPart;
	return 0;
#else
	// fstat(2) sets st_size to 0 on block devices
	struct stat s;
	if (fstat(handle, &s) == -1)
		return 1;
	switch (s.st_mode & __S_IFMT) {
	case __S_IFREG:
	case __S_IFLNK:
		*size = s.st_size;
		return 0;
	case __S_IFBLK:
		//TODO: Non-linux variant
		ioctl(handle, BLKGETSIZE64, size);
		return 0;
	default: return 1;
	}
#endif
}
