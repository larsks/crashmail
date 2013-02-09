#ifndef OS_OSDIR_H
#define OS_OSDIR_H

#include "shared/types.h"

#include <time.h>

struct osFileEntry
{
   struct osFileEntry *Next;
   char Name[100];
   time_t Date;
   uint32_t Size;
};

bool osReadDir(char *dir,struct jbList *filelist,bool (*acceptfunc)(char *filename));
bool osScanDir(char *dir,void (*func)(char *file));
struct osFileEntry *osGetFileEntry(char *file);

#endif
