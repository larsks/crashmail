#ifndef SHARED_TYPES_H
#define SHARED_TYPES_H

#ifdef PLATFORM_LINUX
#define NO_TYPEDEF_ULONG
#define NO_TYPEDEF_USHORT
#endif

#ifndef NO_TYPEDEF_ULONG
typedef unsigned long ulong;
#endif

#ifndef NO_TYPEDEF_USHORT
typedef unsigned short ushort;
#endif

#ifndef NO_TYPEDEF_UCHAR
typedef unsigned char uchar;
#endif

#ifndef NO_TYPEDEF_BOOL
typedef int bool;
#endif

#define FALSE 0
#define TRUE 1

#endif
