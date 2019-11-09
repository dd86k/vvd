#include <stdio.h>
#include "os.h"

void os_perror(const char *func) {
	//TODO: Consider os_perror to return last OS error
#ifdef _WIN32
	fprintf(stderr, "%s: ", func);
	wchar_t buffer[1024];
	unsigned int e = GetLastError();
	int l = GetLocaleInfoEx( // Recommended over MAKELANGID
		LOCALE_NAME_USER_DEFAULT,
		LOCALE_ALL,
		0,
		0);
	FormatMessageW(
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		e,
		l,
		buffer,
		1024,
		NULL);
	fputws(buffer, stderr);
#else
	perror(func);
#endif
}
