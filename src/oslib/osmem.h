#ifndef OS_OSMEM_H
#define OS_OSMEM_H

#include "shared/types.h"

void *osAlloc(ulong size);
void *osAllocCleared(ulong size);
void osFree(void *buf);

#endif