#include <windows.h>

#include <stdlib.h> /* For system() */

#include <shared/types.h>
#include <shared/jblist.h>

#include <oslib/osmisc.h>
#include <oslib/osfile.h>

void osSetComment(uchar *file,uchar *comment)
{
   /* Does not exist in this os */
}

/* Returns -1 if dir was not found and errorlevel otherwise */

int osChDirExecute(uchar *dir,uchar *cmd)
{
   char olddir[300];
   int res;
   
   if(!GetCurrentDirectory(300,olddir))
      return(-1);

   if(!SetCurrentDirectory(dir))
      return(-1);

   res=osExecute(cmd);

   SetCurrentDirectory(olddir);

   return(res);
}


int osExecute(uchar *cmd)
{
   return system(cmd);
}

bool osExists(uchar *file)
{
   HANDLE hFind;
   WIN32_FIND_DATA FindFileData;

   hFind = FindFirstFile(file,&FindFileData);
   FindClose(hFind);

   if (hFind == INVALID_HANDLE_VALUE)
      return(FALSE);

   return(TRUE);
}

bool osMkDir(uchar *dir)
{
   if(CreateDirectory(dir,NULL))
      return(TRUE);

   return(FALSE);
}


bool osRename(uchar *oldfile,uchar *newfile)
{
   if(MoveFile(oldfile,newfile))
		return(TRUE);

	return(FALSE);
}

bool osDelete(uchar *file)
{
   if(DeleteFile(file))
		return(TRUE);
		
	return(FALSE);
}

void osSleep(int secs)
{
   Sleep(secs*1000);
}

uchar *osErrorMsg(ulong errnum)
{
   uchar charbuf[1000];
   static uchar oembuf[1000];

   FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_MAX_WIDTH_MASK,
                 NULL,(DWORD)errnum,0,charbuf,1000,NULL);

   CharToOem(charbuf,oembuf);

   return (uchar *)oembuf;
}

ulong osError(void)
{
   return (ulong)GetLastError();
}



