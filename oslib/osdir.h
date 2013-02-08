#ifndef OS_OSDIR_H
#define OS_OSDIR_H

#include "shared/types.h"

#include <time.h>

struct osFileEntry
{
   struct osFileEntry *Next;
   uchar Name[100];
   time_t Date;
   uint32_t Size;
};

bool osReadDir(uchar *dir,struct jbList *filelist,bool (*acceptfunc)(uchar *filename));
bool osScanDir(uchar *dir,void (*func)(uchar *file));
struct osFileEntry *osGetFileEntry(uchar *file);

#endif
