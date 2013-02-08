#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <shared/types.h>
#include <shared/jblist.h>

#include <oslib/osmisc.h>

#include <sys/stat.h>
#include <sys/wait.h>

#include <unistd.h>

void osSetComment(uchar *file,uchar *comment)
{
   /* Does not exist in this os */
}

/* Returns -1 if dir was not found and errorlevel otherwise */

int osChDirExecute(uchar *dir,uchar *cmd)
{
   char olddir[300];
   int res;
   
   if(!getcwd(olddir,300))
      return(-1);

   if(chdir(dir) != 0)
      return(-1);

   res=osExecute(cmd);
	
   chdir(olddir);

   return(res);
}

int osExecute(uchar *cmd)
{
   int res;
	
   res=system(cmd);

   return WEXITSTATUS(res);
}

bool osExists(uchar *file)
{
   struct stat st;

   if(stat(file,&st) == 0)
      return(TRUE);

   return(FALSE);
}

bool osMkDir(uchar *dir)
{
   if(mkdir(dir,0777) != 0)
      return(FALSE);

   return(TRUE);
}

bool osRename(uchar *oldfile,uchar *newfile)
{
	if(rename(oldfile,newfile) == 0)
		return(TRUE);

	return(FALSE);
}

bool osDelete(uchar *file)
{
	if(remove(file) == 0)
		return(TRUE);
		
	return(FALSE);
}

void osSleep(int secs)
{
   sleep(secs);
}

uchar *osErrorMsg(uint32_t errnum)
{
	return (uchar *)strerror(errnum);
}

uint32_t osError(void)
{
	return (uint32_t)errno;
}
