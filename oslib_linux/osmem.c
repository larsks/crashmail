#include <stdlib.h>
#include <stdint.h>

#include <oslib/osmem.h>

void *osAlloc(uint32_t size)
{
   return malloc((size_t)size);
}

void *osAllocCleared(uint32_t size)
{
   return calloc((size_t)size,1);
}

void osFree(void *buf)
{
   free(buf);
}
