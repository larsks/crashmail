#ifndef OS_OS_H
#define OS_OS_H

#include <shared/types.h>

bool osInit(void);
void osEnd(void);

#if defined(__WIN32__)
 #include <oslib_win32/os_win32.h>
#elif defined(__LINUX__)
 #include <oslib_linux/os_linux.h>
#else
 #error Unsupported platform
#endif

#endif

