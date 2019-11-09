#include <stdio.h>
#include "os.h"
#include "err.h"

__OSFILE os_open(_vchar *path) {
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
	if (fd == INVALID_HANDLE_VALUE) {
		os_perror(__func__);
		return 0;
	}
#else
	__OSFILE fd = open(path, O_RDWR);
	if (fd == -1) {
		os_perror(__func__);
		return 0;
	}
#endif
	return fd;
}

__OSFILE os_create(_vchar *path) {
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
	if (fd == INVALID_HANDLE_VALUE) {
		os_perror(__func__);
		return 0;
	}
#else
	__OSFILE fd = open(path, O_RDWR | O_CREAT | O_TRUNC);
	if (fd == -1) {
		os_perror(__func__);
		return 0;
	}
#endif
	return fd;
}

int os_seek(__OSFILE handle, int64_t pos, int flags) {
#ifdef _WIN32
	LARGE_INTEGER a;
	a.QuadPart = pos;
	if (SetFilePointerEx(handle, a, NULL, flags) == 0) {
		os_perror(__func__);
		return -1;
	}
#else
	if (lseek(handle, (off_t)pos, flags) == -1) {
		os_perror(__func__);
		return -1;
	}
#endif
	return 0;
}

int os_read(__OSFILE handle, void *buffer, size_t size) {
#ifdef _WIN32
	DWORD r;
	if (ReadFile(handle, buffer, size, &r, NULL) == 0) {
		os_perror(__func__);
		return -1;
	}
	if (r != size) {
		fprintf(stderr, "os_read: Failed to read %u/%u bytes",
			(uint32_t)r, (uint32_t)size);
		return -2;
	}
#else
	ssize_t r;
	if ((r = read(handle, buffer, size)) == -1) {
		os_perror(__func__);
		return -1;
	}
	if (r != size) {
		fprintf(stderr, "os_read: Failed to read %d/%u bytes",
			(int32_t)r, (uint32_t)size);
		return -2;
	}
#endif
	return 0;
}

int os_write(__OSFILE handle, void *buffer, size_t size) {
#ifdef _WIN32
	DWORD r;
	if (WriteFile(handle, buffer, size, &r, NULL) == 0) {
		os_perror(__func__);
		return -1;
	}
	if (r != size) {
		fprintf(stderr, "os_write: Failed to write %u bytes", (unsigned int)r);
		return -2;
	}
#else
	ssize_t r;
	if ((r = write(handle, buffer, size)) == -1) {
		os_perror(__func__);
		return -1;
	}
	if (r != size) {
		fprintf(stderr, "os_write: Failed to read %u bytes", (unsigned int)r);
		return -2;
	}
#endif
	return 0;
}
