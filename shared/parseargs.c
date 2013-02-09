#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <oslib/os.h>

#include "shared/parseargs.h"

bool parseargs(struct argument *arg,int argc, char **argv)
{
   int i,j;

   for(i=1;i<argc;i++)
   {
      for(j=0;arg[j].type;j++)
         if(arg[j].name && stricmp(argv[i],arg[j].name)==0) break;

      if(arg[j].type)
      {
         /* Matches a keyword */

         if(arg[j].type == ARGTYPE_STRING)
         {
            if(i+1 < argc)
            {
               /* Has argument */
               arg[j].data=(void *)argv[i+1];
               i++;
            }
            else
            {
               /* Argument missing */
               printf("No argument for %s\n",arg[j].name);
               return(FALSE);
            }
         }
         else if(arg[j].type == ARGTYPE_BOOL)
         {
            arg[j].data=(void *)TRUE;
         }
      }
      else
      {
         /* Fill in first empty field */

         for(j=0;arg[j].type;j++)
            if((!arg[j].name || arg[j].flags & ARGFLAG_AUTO) && !arg[j].data) break;

         if(!arg[j].type)
         {
            printf("Unknown keyword %s\n",argv[i]);
            return(FALSE);
         }

         arg[j].data=(void *)argv[i];
      }
   }

   for(j=0;arg[j].type;j++)
      if((arg[j].flags & ARGFLAG_MANDATORY) && !arg[j].data)
		{
			printf("Mandatory argument %s not specified\n",arg[j].name);
			return(FALSE);
		}

   return(TRUE);
}

void printargs(struct argument *arg)
{
   int j;
	char buf1[50],buf2[50];
	
   printf("\nAvailable arguments:\n\n");

   for(j=0;arg[j].type;j++)
      if(arg[j].name)
      {
			if(arg[j].flags & ARGFLAG_AUTO) sprintf(buf1,"(%s)",arg[j].name);
			else strcpy(buf1,arg[j].name);
			
			buf2[0]=0;
			if(arg[j].flags & ARGFLAG_MANDATORY) strcpy(buf2," [Mandatory]");

         switch(arg[j].type)
         {
            case ARGTYPE_STRING:
               printf("%s <string>%s\n",buf1,buf2);
               break;
            case ARGTYPE_BOOL:
               printf("%s\n",arg[j].name);      
               break;
         }
      }

   printf("\n");
	printf("[Mandatory] means that the argument has to be specified. Parentheses around\n");
	printf("the keyword mean that the keyword itself does not have to be specified.\n\n");
}

