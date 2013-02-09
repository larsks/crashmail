#include <windows.h>

#include <stdio.h> /* Only for vsprintf() */
#include <shared/types.h>

#include <oslib/osfile.h>
#include <oslib/osmem.h>

#define MIN(a,b)  ((a)<(b)? (a):(b))

#define OSFILE_BUFSIZE 5000

#define LASTACCESS_READ   1
#define LASTACCESS_WRITE  2

struct osfile
{
   HANDLE h;
   char *buf;
   int lastaccessmode;
   ulong bufmax;
   ulong buflen;
   ulong bufpos;
   ulong filepos;
};

bool win32writebuffer(struct osfile *osf)
{
   DWORD numberofbyteswritten;

   if(osf->bufpos == 0)
      return(TRUE); /* Nothing to do */

   if(!WriteFile(osf->h,osf->buf,osf->bufpos,&numberofbyteswritten,NULL))
      return(FALSE);

   if(numberofbyteswritten != osf->bufpos)
      return(FALSE);

   osf->bufpos=0;
   osf->buflen=0;

   return(TRUE);
}

bool win32readbuffer(struct osfile *osf)
{
   DWORD numberofbytesread;

   if(!ReadFile(osf->h,osf->buf,osf->bufmax,&numberofbytesread,NULL))
      return(FALSE);

   if(numberofbytesread == 0)
      return(FALSE);

   osf->buflen=numberofbytesread;
   osf->bufpos=0;

   return(TRUE);
}

bool win32setmoderead(struct osfile *osf)
{
   if(osf->lastaccessmode == LASTACCESS_WRITE)
   {
      /* Flush write buffer */

      if(!win32writebuffer(osf))
         return(FALSE);

      osf->buflen=0;
      osf->bufpos=0;
   }

   osf->lastaccessmode = LASTACCESS_READ;
   return(TRUE);
}

bool win32setmodewrite(struct osfile *osf)
{
   if(osf->lastaccessmode == LASTACCESS_READ)
   {
      /* File pointer needs to be adjusted */

      SetFilePointer(osf->h,osf->filepos,NULL,FILE_END);

      osf->buflen=0;
      osf->bufpos=0;
   }

   osf->lastaccessmode = LASTACCESS_WRITE;
   return(TRUE);
}

osFile osOpen(char *name,ulong mode)
{
   DWORD desiredaccess;
   DWORD sharemode;
   DWORD creationdisposition;
   struct osfile *os;

   if(!(os=osAllocCleared(sizeof(struct osfile))))
      return(NULL);

   if(!(os->buf=osAlloc(OSFILE_BUFSIZE)))
   {
      osFree(os);
      return(NULL);
   }

   if(mode == MODE_NEWFILE) 
	{
      desiredaccess=GENERIC_WRITE;
      sharemode=0;
      creationdisposition=CREATE_ALWAYS;
	}
   else if(mode == MODE_OLDFILE) 
	{
      desiredaccess=GENERIC_READ;
      sharemode=FILE_SHARE_READ;
      creationdisposition=OPEN_EXISTING;
  	}
	else
	{
      desiredaccess=GENERIC_READ | GENERIC_WRITE;
      sharemode=FILE_SHARE_READ | FILE_SHARE_WRITE;
      creationdisposition=OPEN_ALWAYS;
   }

   os->h=CreateFile(name,desiredaccess,sharemode,NULL,
                    creationdisposition,FILE_ATTRIBUTE_NORMAL,NULL);

   if(os->h == INVALID_HANDLE_VALUE)
   {
      osFree(os->buf);
      osFree(os);
      return(NULL);
   }

   os->bufmax=OSFILE_BUFSIZE;

   return (osFile)os;
}

void osClose(osFile os)
{
   struct osfile *osf=(struct osfile *)os;

   if(osf->lastaccessmode == LASTACCESS_WRITE)
      win32writebuffer(osf);

   CloseHandle(osf->h);
   osFree(osf->buf);
   osFree(osf);
}

ulong osRead(osFile os,void *buf,ulong bytes)
{
   struct osfile *osf=(struct osfile *)os;
   ulong read,n;

   win32setmoderead(osf);

   read=0;

   if(osf->buflen-osf->bufpos != 0)
   {
      /* Copy what's left in the buffer */

      n=MIN(osf->buflen-osf->bufpos,bytes);

      memcpy(buf,&osf->buf[osf->bufpos],n);
      osf->bufpos+=n;
      osf->filepos+=n;

      buf+=n;
      bytes-=n;
      read+=n;
   }

   if(bytes > osf->bufmax)
   {
      /* Larger than buffer, read directly */

      DWORD numberofbytesread;

      if(!ReadFile(osf->h,buf,bytes,&numberofbytesread,NULL))
         return(read);

      read+=numberofbytesread;
      osf->filepos+=numberofbytesread;
   }
   else if(bytes > 0)
   {
      /* Get rest from buffer */

      if(!win32readbuffer(osf))
         return(read);

      n=MIN(osf->buflen-osf->bufpos,bytes);

      memcpy(buf,&osf->buf[osf->bufpos],n);
      osf->bufpos+=n;
      osf->filepos+=n;

      buf+=n;
      bytes-=n;
      read+=n;
   }

   return(read);
}

int osGetChar(osFile os)
{
   struct osfile *osf=(struct osfile *)os;

   win32setmoderead(osf);

   if(osf->buflen-osf->bufpos == 0)
   {
      /* Buffer needs to be refilled */

      if(!win32readbuffer(osf))
         return(-1);
   }

   osf->filepos++;
   return(osf->buf[osf->bufpos++]);
}

ulong osFGets(osFile os,char *str,ulong max)
{
   ulong d;
   int ch;

   d=0;

   if(max==0)
      return(0);

   for(;;)
   {
      ch=osGetChar(os);

      if(ch == -1) /* End of file */
      {
         str[d]=0;
         return(d);
      }

      if(ch == 10) /* End of line */
      {
         str[d++]=ch;
         str[d]=0;
         return(d);
      }

      if(ch != 13) /* CRs are skipped */
         str[d++]=ch;

      if(d == max-1) /* Buffer full */
      {
         str[d]=0;
         return(d);
      }
   }
}

bool osWrite(osFile os,const void *buf,ulong bytes)
{
   struct osfile *osf=(struct osfile *)os;
   DWORD numberofbyteswritten;

   win32setmodewrite(osf);

   if(bytes > osf->bufmax-osf->bufpos)
   {
      /* Will not fit in buffer */

      if(!win32writebuffer(osf))
         return(FALSE);
   }

   if(bytes > osf->bufmax)
   {
      /* Larger than buffer, write directly */

      if(!WriteFile(osf->h,buf,bytes,&numberofbyteswritten,NULL))
         return(FALSE);

      if(numberofbyteswritten != bytes)
         return(FALSE);

      osf->filepos+=bytes;
   }
   else
   {
      /* Put in buffer */

      memcpy(&osf->buf[osf->bufpos],buf,bytes);
      osf->bufpos+=bytes;
      osf->filepos+=bytes;
   }

   return(TRUE);
}

bool osPutChar(osFile os, char ch)
{
   struct osfile *osf=(struct osfile *)os;

   win32setmodewrite(osf);

   if(osf->bufpos == osf->bufmax)
   {
      if(!win32writebuffer(osf))
         return(FALSE);
   }

   osf->buf[osf->bufpos++]=ch;
   osf->filepos++;

   return(TRUE);
}

bool osPuts(osFile os,char *str)
{
   if(osWrite(os,str,strlen(str))!=strlen(str))
		return(FALSE);
		
	return(TRUE);
}

bool osFPrintf(osFile os,char *fmt,...)
{
   va_list args;
   char buf[1000];
	
   va_start(args, fmt);
   vsprintf(buf,fmt,args);
   va_end(args);

   return osPuts(os,buf);
}

bool osVFPrintf(osFile os,char *fmt,va_list args)
{
   char buf[1000];
	
   vsprintf(buf,fmt,args);

   return osPuts(os,buf);
}

void osSeek(osFile os,ulong offset,short mode)
{
   struct osfile *osf=(struct osfile *)os;
   DWORD movemethod;

   /* Flush buffer if needed */

   if(osf->lastaccessmode == LASTACCESS_WRITE)
      win32writebuffer(osf);

   /* Empty buffer */

   osf->buflen=0;
   osf->bufpos=0;
   osf->lastaccessmode=0;

   if(mode == OFFSET_BEGINNING)
      movemethod=FILE_BEGIN;

   if(mode == OFFSET_END)
      movemethod=FILE_END;

   osf->filepos=SetFilePointer(osf->h,offset,NULL,movemethod);
}

ulong osFTell(osFile os)
{
   struct osfile *osf=(struct osfile *)os;

   return(osf->filepos);
}
