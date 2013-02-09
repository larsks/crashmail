#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

#include <dirent.h>
#include <sys/stat.h>

#include <shared/types.h>
#include <shared/jblist.h>
#include <shared/mystrncpy.h>
#include <shared/path.h>

#include <oslib/osmem.h>
#include <oslib/osdir.h>

bool osReadDir(char *dirname,struct jbList *filelist,bool (*acceptfunc)(char *filename))
{
   DIR *dir;
   struct dirent *dirent;
   struct osFileEntry *tmp;
   char buf[200];

   jbNewList(filelist);

   if(!(dir=opendir(dirname)))
      return(FALSE);

   while((dirent=readdir(dir)))
   {
      bool add;

      if(!acceptfunc)   add=TRUE;
      else              add=(*acceptfunc)(dirent->d_name);

      if(add)
      {
         struct stat st;

         MakeFullPath(dirname,dirent->d_name,buf,200);

			if(stat(buf,&st) == 0)
	 		{
            if(!(tmp=(struct osFileEntry *)osAllocCleared(sizeof(struct osFileEntry))))
            {
               jbFreeList(filelist);
               closedir(dir);
               return(FALSE);
            }

            mystrncpy(tmp->Name,dirent->d_name,100);
            tmp->Size=st.st_size;
            tmp->Date=st.st_mtime;

            jbAddNode(filelist,(struct jbNode *)tmp);
         }
      }
   }

   closedir(dir);

   return(TRUE);
}

bool osScanDir(char *dirname,void (*func)(char *file))
{
   DIR *dir;
   struct dirent *dirent;

   if(!(dir=opendir(dirname)))
      return(FALSE);

   while((dirent=readdir(dir)))
      (*func)(dirent->d_name);

   closedir(dir);

   return(TRUE);
}

struct osFileEntry *osGetFileEntry(char *file)
{
   struct stat st;
   struct osFileEntry *tmp;

   if(stat(file,&st) != 0)
      return(FALSE);

   if(!(tmp=(struct osFileEntry *)osAllocCleared(sizeof(struct osFileEntry))))
      return(FALSE);

   mystrncpy(tmp->Name,GetFilePart(file),100);

   tmp->Size=st.st_size;
   tmp->Date=st.st_mtime;

   return(tmp);
}
