#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <shared/types.h>
#include <shared/mystrncpy.h>
#include <oslib/os.h>

void MakeFullPath(char *path,char *file,char *dest,uint32_t destsize)
{
   int d;
   char *chr;

   chr=OS_PATH_CHARS;

   mystrncpy(dest,path,destsize);
   d=strlen(dest);

   if(d != 0)
   {
      if(!strchr(chr,dest[d-1]))
         if(d+1 < destsize)
         {
            dest[d++]=(char)chr[0];
            dest[d]=0;
         }
   }

   if(destsize-d-1 > 0)
      mystrncpy(&dest[d],file,destsize-d-1);
}

char *GetFilePart(char *str)
{
   int d;
   char *chr,*ret;

   chr=OS_PATH_CHARS;

   ret=str;

   for(d=0;str[d];d++)
      if(strchr(chr,str[d])) ret = &str[d+1];

   return(ret);
}












