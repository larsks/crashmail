#include <stdio.h>
#include <string.h>

#include <os/os.h>

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
            if(!arg[j].name && !arg[j].data) break;

         if(!arg[j].type)
         {
            printf("Too many arguments\n");
            return(FALSE);
         }

         arg[j].data=(void *)argv[i];
      }
   }

   return(TRUE);
}

void printargs(struct argument *arg)
{
   int j;

   printf("\nAvailable arguments:\n\n");

   for(j=0;arg[j].type;j++)
      if(arg[j].name)
      {
         switch(arg[j].type)
         {
            case ARGTYPE_STRING:
               printf("%s <string>\n",arg[j].name);
               break;
            case ARGTYPE_BOOL:
               printf("%s\n",arg[j].name);      
               break;
         }
      }

   printf("\n");
}

