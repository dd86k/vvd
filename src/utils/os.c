#include <stdio.h>
#include "os.h"
#ifndef _WIN32
#include <string.h>	// memset
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/fs.h>
#endif

//
// os_fopen
//

__OSFILE os_fopen(const oschar *path) {
#ifdef _WIN32
	__OSFILE fd = CreateFileW(
		path,	// lpFileName
		GENERIC_READ | GENERIC_WRITE,	// dwDesiredAccess
		0,	// dwShareMode
		NULL,	// lpSecurityAttributes
		OPEN_EXISTING,	// dwCreationDisposition
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

//
// os_fcreate
//

__OSFILE os_fcreate(const oschar *path) {
#ifdef _WIN32
	__OSFILE fd = CreateFileW(
		path,	// lpFileName
		GENERIC_READ | GENERIC_WRITE,	// dwDesiredAccess
		0,	// dwShareMode
		NULL,	// lpSecurityAttributes
		CREATE_ALWAYS,	// dwCreationDisposition
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

//
// os_fseek
//

int os_fseek(__OSFILE fd, int64_t pos, int flags) {
#ifdef _WIN32
	LARGE_INTEGER a;
	a.QuadPart = pos;
	if (SetFilePointerEx(fd, a, NULL, flags) == 0)
		return -1;
#else
	if (lseek(fd, (off_t)pos, flags) == -1)
		return -1;
#endif
	return 0;
}

//
// os_fread
//

int os_fread(__OSFILE fd, void *buffer, size_t size) {
#ifdef _WIN32
	DWORD r;
	if (ReadFile(fd, buffer, size, &r, NULL) == 0)
		return -1;
	/*if (r != size) {
		fprintf(stderr, "os_fread: Failed to read %u/%u bytes",
			(uint32_t)r, (uint32_t)size);
		return -2;
	}*/
#else
	ssize_t r;
	if ((r = read(fd, buffer, size)) == -1)
		return -1;
	/*if (r != size) {
		fprintf(stderr, "os_fread: Failed to read %d/%u bytes",
			(int32_t)r, (uint32_t)size);
		return -2;
	}*/
#endif
	return 0;
}

//
// os_fwrite
//

int os_fwrite(__OSFILE fd, void *buffer, size_t size) {
#ifdef _WIN32
	DWORD r;
	if (WriteFile(fd, buffer, size, &r, NULL) == 0)
		return -1;
	/*if (r != size)
		return -2;*/
#else
	ssize_t r;
	if ((r = write(fd, buffer, size)) == -1)
		return -1;
	/*if (r != size)
		return -2;*/
#endif
	return 0;
}

//
// os_fsize
//

int os_fsize(__OSFILE fd, uint64_t *size) {
#if _WIN32
	LARGE_INTEGER li;
	// NOTE: Doc says 0 (FALSE) on failure which apparently doesn't.
	// NOTE: Not supported in Windows Store Apps (use GetFileInformationByHandleEx)
	if (GetFileSizeEx(fd, &li))
		return -1;
	*size = li.QuadPart;
	return 0;
#else
	// fstat(2) sets st_size to 0 on block devices
	struct stat s;
	if (fstat(fd, &s) == -1)
		return 1;
	switch (s.st_mode & __S_IFMT) {
	case __S_IFREG:
	case __S_IFLNK:
		*size = s.st_size;
		return 0;
	case __S_IFBLK:
		//TODO: Non-linux variant
		ioctl(fd, BLKGETSIZE64, size);
		return 0;
	default: return -1;
	}
#endif
}

//
// os_falloc
//

int os_falloc(__OSFILE fd, uint64_t fsize) {
	const int bsize = 1024 * 1024; // 1 MiB
	uint8_t *buf = calloc(1, bsize); // zeroed
	if (fsize >= bsize) {
		if (buf == NULL)
			return 1;
		while (fsize > bsize) {
			if (os_fwrite(fd, buf, bsize))
				return -1;
			fsize -= bsize;
		}
	}
	if (fsize > 0) {
		if (os_fwrite(fd, buf, bsize - fsize))
			return -1;
	}
	free(buf);
	return 0;
}

//
// os_pinit
//

static const uint32_t OS_P_ALLOC = 2048;

int os_pinit(struct progress_t *p, uint32_t flags, uint32_t max) {
#if _WIN32
	p->fd = GetStdHandle(STD_OUTPUT_HANDLE);

	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(p->fd, &csbi);
	p->leny = (csbi.srWindow.Bottom - csbi.srWindow.Top + 1);
	p->lenx = (csbi.srWindow.Right - csbi.srWindow.Left + 1);
	p->inity = csbi.dwCursorPosition.Y;
	p->initx = csbi.dwCursorPosition.X;
#else
	struct winsize ws;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
	p->leny = ws.ws_row;
	p->lenx = ws.ws_col;
#endif
	if ((p->bfill = malloc(OS_P_ALLOC << 1)) == NULL) // *2
		return 2;
	p->bspace = p->bfill + OS_P_ALLOC;
	memset(p->bfill, '=', OS_P_ALLOC);
	memset(p->bspace, ' ', OS_P_ALLOC);
	p->maximum = max;
	p->flags = flags;

	return 0;
}

//
// os_pupdate
//

int os_pupdate(struct progress_t *p, uint32_t val) {
#if _WIN32
	COORD c;
	c.X = 0;
	c.Y = p->inity;
	SetConsoleCursorPosition(p->fd, c);
#else
	//printf("\033[%d;%dH", p->inity, 0);
	fputs("\r", stdout); // "CSI2n" clears line
#endif
	float cc = (float)val / p->maximum; // current
	int pc; // characters printed
	uint32_t ph;	// placeholder
	switch (p->flags & 0xF) {
	case PROG_MODE_CUR_MAX:
		pc = printf("%u/%u [", val, p->maximum);
		break;
	case PROG_MODE_CUR_ONLY:
		pc = printf("%u [", val);
		break;
	case PROG_MODE_POURCENT:
		ph = (uint32_t)(cc * 100.0f);
		/*if (ph == ((float)p->current / p->maximum) * 100.0f) { // same %
			p->current = val;
			return 0; // NOP
		}*/
		pc = printf("%u%% [", ph);
		break;
	default:
		pc = printf("[");
		break;
	}
	if (pc < 0)
		return -11;

	uint32_t total = p->lenx - pc - 2;
	uint32_t occup = cc * total;	// filled
	uint32_t space = total - occup;	// what's left
	printf("%.*s%.*s]", occup, p->bfill, space, p->bspace);

	p->current = val;
	return 0;
}

//
// os_pfinish
//

int os_pfinish(struct progress_t *p) {
	if (p->flags & PROG_FLAG_REMOVE) {
#if _WIN32
		COORD c;
		c.X = 0;
		c.Y = p->inity;
		SetConsoleCursorPosition(p->fd, c);
#else
		fputs("\r", stdout);
#endif
		printf("%.*s", p->lenx, p->bspace);
	}
	free(p->bfill);
	free(p->bspace);
	putchar('\n');
	return 0;
}
