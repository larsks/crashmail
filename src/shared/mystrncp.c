#include "shared/types.h"
#include <string.h>

void mystrncpy(uchar *dest,uchar *src,ulong len)
{
   strncpy(dest,src,(size_t)len-1);
   dest[len-1]=0;
}

