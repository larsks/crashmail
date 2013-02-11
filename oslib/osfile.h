#ifndef OS_OSFILE_H
#define OS_OSFILE_H

#include <stdarg.h>

#include "shared/types.h"

typedef void *osFile;

#define MODE_OLDFILE    1005  /* Corresponds to "rb" with fopen */
#define MODE_NEWFILE    1006  /* Corresponds to "wb" with fopen */
#define MODE_READWRITE  1004  /* Corresponds to "w+b" with fopen */

#define OFFSET_BEGINNING -1
#define OFFSET_END	 1 

osFile osOpen(char *name,uint32_t mode);
void osClose(osFile os);
int osGetChar(osFile os);
uint32_t osRead(osFile os,void *buf,uint32_t bytes);
uint32_t osFGets(osFile os,char *str,uint32_t max);
off_t osFTell(osFile os);
bool osPutChar(osFile os, char ch);
bool osWrite(osFile os,const void *buf, uint32_t bytes);
bool osPuts(osFile os,char *str);
bool osFPrintf(osFile os,char *fmt,...);
bool osVFPrintf(osFile os,char *fmt,va_list args);
void osSeek(osFile os,off_t offset,short mode);

#endif
