#ifndef OS_OSMEM_H
#define OS_OSMEM_H

#include "shared/types.h"

void *osAlloc(uint32_t size);
void *osAllocCleared(uint32_t size);
void osFree(void *buf);

#endif
