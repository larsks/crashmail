#ifndef OS_OSFILE_H
#define OS_OSFILE_H

#include "shared/types.h"

typedef void *osFile;

#define MODE_OLDFILE    1005  /* Corresponds to "rb" with fopen */
#define MODE_NEWFILE    1006  /* Corresponds to "wb" with fopen */
#define MODE_READWRITE  1004  /* Corresponds to "w+b" with fopen */

#define OFFSET_BEGINNING -1
#define OFFSET_END	 1 

osFile osOpen(uchar *name,ulong mode);
void osClose(osFile os);
int osGetChar(osFile os);
ulong osRead(osFile os,void *buf,ulong bytes);
void osPutChar(osFile os, uchar ch);
void osWrite(osFile os,const void *buf, ulong bytes);
void osPuts(osFile os,uchar *str);
ulong osFGets(osFile os,uchar *str,ulong max);
ulong osFTell(osFile os);
void osFPrintf(osFile os,uchar *fmt,...);
void osSeek(osFile os,ulong offset,short mode);

#endif
