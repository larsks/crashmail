#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include <shared/types.h>

#include <oslib/osfile.h>

extern bool nomem,diskfull;

/* OBS! Klarar _inte_ Read/Write mode */

osFile osOpen(uchar *name,ulong mode)
{
   FILE *fh;

   if(mode == MODE_NEWFILE)
      return (osFile) fopen(name,"wb");

   if(mode == MODE_OLDFILE)
      return (osFile )fopen(name,"rb");

   /* MODE_READWRITE */
   
   if(!(fh=fopen(name,"r+b")))
      fh=fopen(name,"w+b");

   return (osFile) fh;
}

void osClose(osFile os)
{
    fclose((FILE *)os);
}

int osGetChar(osFile os)
{
   int c;

   c=fgetc((FILE *)os);

   if(c==EOF)
      c=-1;

   return(c);
}

ulong osRead(osFile os,void *buf,ulong bytes)
{
   return fread(buf,1,bytes,(FILE *)os);
}

void osPutChar(osFile os, uchar ch)
{
   if(fputc(ch,(FILE *)os)==EOF)
      diskfull=TRUE;
}

void osWrite(osFile os,const void *buf,ulong bytes)
{
   if(fwrite(buf,1,bytes,(FILE *)os)!=bytes)
      diskfull=TRUE;
}

void osPuts(osFile os,uchar *str)
{
   if(fputs(str,(FILE *)os)==EOF)
      diskfull=TRUE;
}

ulong osFGets(osFile os,uchar *str,ulong max)
{
   char *s;

   s=fgets(str,max,(FILE *)os);

   if(s)
   {
      if(strlen(s)>=2 && s[strlen(s)-1]==10 && s[strlen(s)-2]==13)
      {
         /* CRLF -> LF */

         s[strlen(s)-2]=10;
         s[strlen(s)-1]=0;
      }

      return (ulong)strlen(s);
   }

   return(0);
}

ulong osFTell(osFile os)
{
   return ftell((FILE *)os);
}

void osFPrintf(osFile os,uchar *fmt,...)
{
   va_list args;

   va_start(args, fmt);
   vfprintf(os,fmt,args);
   va_end(args);
}

void osSeek(osFile fh,ulong offset,short mode)
{
   int md;

   if(mode == OFFSET_BEGINNING)
      md=SEEK_SET;

   if(mode == OFFSET_END)
      md=SEEK_END;

   fseek((FILE *)fh,offset,md);
}
