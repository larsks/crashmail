#include <windows.h>

#include <shared/types.h>
#include <shared/jblist.h>
#include <shared/mystrncpy.h>
#include <shared/path.h>

#include <oslib/osmem.h>
#include <oslib/osdir.h>

void win32finddata_to_fileentry(WIN32_FIND_DATA *w32,struct osFileEntry *fe)
{
   FILETIME local;
   SYSTEMTIME systime;
   struct tm tp;

   mystrncpy(fe->Name,w32->cFileName,100);

   fe->Size=w32->nFileSizeLow;

   FileTimeToLocalFileTime(&w32->ftLastWriteTime,&local);
   FileTimeToSystemTime(&local,&systime);

   tp.tm_sec=systime.wSecond;
   tp.tm_min=systime.wMinute;
   tp.tm_hour=systime.wHour;
   tp.tm_mday=systime.wDay;
   tp.tm_mon=systime.wMonth-1;
   tp.tm_year=systime.wYear-1900;
   tp.tm_wday=0;
   tp.tm_yday=0;
   tp.tm_isdst=-1;

   fe->Date=mktime(&tp);

   if(fe->Date == -1) /* Just in case */
      time(&fe->Date);
}

bool win32readdiraddfile(WIN32_FIND_DATA *FindFileData,struct jbList *filelist,bool (*acceptfunc)(char *filename))
{
   bool add;
   struct osFileEntry *tmp;

   if(!acceptfunc)   add=TRUE;
   else              add=(*acceptfunc)(FindFileData->cFileName);

   if(add)
   {
      if(!(tmp=(struct osFileEntry *)osAllocCleared(sizeof(struct osFileEntry))))
      {
         jbFreeList(filelist);
         return(FALSE);
      }

      win32finddata_to_fileentry(FindFileData,tmp);
      jbAddNode(filelist,(struct jbNode *)tmp);
   }

   return(TRUE);
}

bool osReadDir(char *dirname,struct jbList *filelist,bool (*acceptfunc)(char *filename))
{
   WIN32_FIND_DATA FindFileData;
   HANDLE hFind;
   char buf[200];

   jbNewList(filelist);

   mystrncpy(buf,dirname,190);

   if(buf[strlen(buf)-1] != '\\')
      strcat(buf,"\\");

   strcat(buf,"*");

   hFind = FindFirstFile(buf,&FindFileData);

   if (hFind == INVALID_HANDLE_VALUE)
      return(FALSE);

   if(!win32readdiraddfile(&FindFileData,filelist,acceptfunc))
   {
      FindClose(hFind);
      return(FALSE);
   }

   while(FindNextFile(hFind,&FindFileData))
   {
      if(!win32readdiraddfile(&FindFileData,filelist,acceptfunc))
      {
         FindClose(hFind);
         return(FALSE);
      }
   }

   FindClose(hFind);

   return(TRUE);
}

bool osScanDir(char *dirname,void (*func)(char *file))
{
   WIN32_FIND_DATA FindFileData;
   HANDLE hFind;
   char buf[200];

   mystrncpy(buf,dirname,190);

   if(buf[strlen(buf)-1] != '\\')
      strcat(buf,"\\");

   strcat(buf,"*");

   hFind = FindFirstFile(buf,&FindFileData);

   if (hFind == INVALID_HANDLE_VALUE)
      return(FALSE);

   (*func)(FindFileData.cFileName);

   while(FindNextFile(hFind,&FindFileData))
      (*func)(FindFileData.cFileName);

   FindClose(hFind);

   return(TRUE);
}

struct osFileEntry *osGetFileEntry(char *file)
{
   WIN32_FIND_DATA FindFileData;
   HANDLE hFind;
   struct osFileEntry *tmp;

   hFind = FindFirstFile(file,&FindFileData);
   FindClose(hFind);

   if (hFind == INVALID_HANDLE_VALUE)
      return(NULL);

   if(!(tmp=(struct osFileEntry *)osAllocCleared(sizeof(struct osFileEntry))))
      return(NULL);

   win32finddata_to_fileentry(&FindFileData,tmp);

   return(tmp);
}
