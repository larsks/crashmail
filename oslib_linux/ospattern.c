#include <string.h>
#include <ctype.h>

#include <oslib/os.h>
#include <oslib/ospattern.h>

bool osCheckPattern(char *pattern)
{
   return(TRUE);
}

bool osMatchPattern(char *pattern,char *str)
{
   int c;

   for(c=0;pattern[c];c++)
   {
      if(pattern[c]=='*')
         return(TRUE);

      if(str[c] == 0)
         return(FALSE);

      if(pattern[c]!='?' && tolower(pattern[c])!=tolower(str[c]))
         return(FALSE);
   }

   if(str[c]!=0)
      return(FALSE);

   return(TRUE);
}

bool osIsPattern(char *pat)
{
   if(strchr(pat,'?') || strchr(pat,'*'))
      return(TRUE);

	return(FALSE);
}
