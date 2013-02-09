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

void osSetComment(char *file,char *comment)
{
   /* Does not exist in this os */
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

   res=osExecute(cmd);
	
   chdir(olddir);

   return(res);
}

int osExecute(char *cmd)
{
   int res;
	
   res=system(cmd);

   return WEXITSTATUS(res);
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
   if(mkdir(dir,0777) != 0)
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

char *osErrorMsg(uint32_t errnum)
{
	return (char *)strerror(errnum);
}

uint32_t osError(void)
{
	return (uint32_t)errno;
}
