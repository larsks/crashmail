#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <dir.h>

#include <shared/types.h>
#include <shared/jblist.h>

#include <oslib/osmisc.h>
#include <oslib/osfile.h>

#include <sys/stat.h>

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

   res=system(cmd);

   chdir(olddir);

   return(res);
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
   if(mkdir(dir) != 0)
      return(FALSE);

   return(TRUE);
}

void osSleep(int secs)
{
   sleep(secs*1000);
}


