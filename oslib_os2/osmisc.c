#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <shared/types.h>
#include <shared/jblist.h>

#include <oslib/osmisc.h>
#include <oslib/osfile.h>

#include <sys/types.h>
#include <sys/stat.h>
/*#include <sys/dir.h>*/

#include <os2.h>
#include <io.h>
#include <sys/nls.h>
#include <sys/ead.h>

void osSetComment(char *file,char *comment)
{
   /* Modeled after
    * eatool.c (emx+gcc) -- Copyright (c) 1992-1995 by Eberhard Mattes
    * by Peter Karlsson 1999
    */

   char *buf;
   _ead ead;
   int size;

   ead = _ead_create();
   if (!ead) return;
   size = strlen(comment);
   buf = malloc(size + 4);
   if (buf != NULL)
   {
      ((USHORT *)buf)[0] = EAT_ASCII;
      ((USHORT *)buf)[1] = size;
      memcpy(buf+4, comment, size);
      if (_ead_add(ead, ".SUBJECT", 0, buf, size + 4) >= 0)
      {
         _ead_write(ead, file, 0, _EAD_MERGE);
      }
   }

   free(buf);
   _ead_destroy(ead);
}

/* Returns -1 if dir was not found and errorlevel otherwise */

int osChDirExecute(char *dir,char *cmd)
{
   char olddir[300];
   int res;

   if(!getcwd(olddir,300))
      return(-1);

   if(chdir(dir) != 0)
      return(-1);

   res=system(cmd);

   chdir(olddir);

   return(res);
}


int osExecute(char *cmd)
{
   return system(cmd);
}

bool osExists(char *file)
{
   struct stat st;

   if(stat(file,&st) == 0)
      return(TRUE);

   return(FALSE);
}

bool osMkDir(char *dir)
{
   if(mkdir(dir, 0) != 0)
      return(FALSE);

   return(TRUE);
}


bool osRename(char *oldfile,char *newfile)
{
	if(rename(oldfile,newfile) == 0)
		return(TRUE);

	return(FALSE);
}

bool osDelete(char *file)
{
	if(remove(file) == 0)
		return(TRUE);
		
	return(FALSE);
}

void osSleep(int secs)
{
   sleep(secs);
}


char *osErrorMsg(ulong errnum)
{
	return (char *)strerror(errnum);
}

ulong osError(void)
{
	return (ulong)errno;
}

