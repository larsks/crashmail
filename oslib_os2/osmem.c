#include <stdlib.h>

#include <oslib/osmem.h>

void *osAlloc(ulong size)
{
   return malloc((size_t)size);
}

void *osAllocCleared(ulong size)
{
   return calloc((size_t)size,1);
}

void osFree(void *buf)
{
   free(buf);
}
