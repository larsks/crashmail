#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "shared/types.h"

void mystrncpy(char *dest,char *src,uint32_t len)
{
   strncpy(dest,src,(size_t)len-1);
   dest[len-1]=0;
}

