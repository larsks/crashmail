#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <ctype.h>

#include <oslib/os.h>
#include <oslib/osfile.h>

#include <shared/types.h>
#include <shared/jbstrcpy.h>
#include <shared/parseargs.h>

#define VERSION "1.0"

#ifdef PLATFORM_AMIGA
uchar *ver="$VER: CrashForward " VERSION " " __AMIGADATE__;
#endif

bool diskfull;

uchar cfgbuf[4000];

uchar areabuf[1000];
uchar areagroup;
uchar tagname[100];

#define ARG_PREFSFILE    0
#define ARG_FWDFILE      1
#define ARG_GROUP  	 2
#define ARG_NODESC       3

struct argument args[] =
   { { ARGTYPE_STRING, NULL,           NULL },
     { ARGTYPE_STRING, NULL,           NULL },
     { ARGTYPE_STRING, "GROUP",        NULL },
     { ARGTYPE_BOOL,   "NODESC",       0    },
     { ARGTYPE_END,     NULL,          0    } };

bool CheckFlags(uchar group,uchar *node)
{
   int c;

   for(c=0;c<strlen(node);c++)
   {
      if(toupper(group)==toupper(node[c]))
         return(TRUE);
    }

   return(FALSE);
}

void printarea(osFile fh)
{
   if(stricmp(tagname,"DEFAULT")==0 || strnicmp(tagname,"DEFAULT_",8)==0)
      return;

   if(stricmp(tagname,"BAD")==0)
      return;

   if(!args[ARG_GROUP].data || CheckFlags(areagroup,args[ARG_GROUP].data))
      osFPrintf(fh,"%s\n",areabuf);
}

int main(int argc, char **argv)
{
   osFile ifh,ofh;
   uchar cfgword[30],buf[100];
   ulong jbcpos;  

   if(!osInit())
      exit(OS_EXIT_ERROR);

   if(argc == 2 && strcmp(argv[1],"?")==0)
   {
      printargs(args);
      osEnd();
      exit(OS_EXIT_OK);
   }

   if(!parseargs(args,argc,argv))
   {
      osEnd();
      exit(OS_EXIT_ERROR);
   }
   
   if(!args[ARG_PREFSFILE].data)
   {
      printf("No crashmail.prefs file specified\n");
      osEnd();
      exit(OS_EXIT_ERROR);
   }   

   if(!args[ARG_FWDFILE].data)
   {
      printf("No output forwards file specified\n");
      osEnd();
      exit(OS_EXIT_ERROR);
   }   

   if(!(ifh=osOpen(args[ARG_PREFSFILE].data,MODE_OLDFILE)))
   {
      printf("Failed to open %s for reading\n",(char *)args[ARG_PREFSFILE].data);
      osEnd();
      exit(OS_EXIT_ERROR);
   }

   if(!(ofh=osOpen(args[ARG_FWDFILE].data,MODE_NEWFILE)))
   {
      printf("Failed to open %s for writing\n",(char *)args[ARG_FWDFILE].data);
      osClose(ifh);
      osEnd();
      exit(OS_EXIT_ERROR);
   }

   while(osFGets(ifh,cfgbuf,4000))
   {
      jbcpos=0;
      jbstrcpy(cfgword,cfgbuf,30,&jbcpos);

      if(stricmp(cfgword,"AREA")==0)
      {
	 if(areabuf[0])
            printarea(ofh);

	 areagroup=0;
	
         jbstrcpy(tagname,cfgbuf,100,&jbcpos);
         strcpy(areabuf,tagname);
      }

      if(stricmp(cfgword,"GROUP")==0)
      {
         if(jbstrcpy(buf,cfgbuf,100,&jbcpos))
            areagroup=buf[0];
      }   

      if(stricmp(cfgword,"DESCRIPTION")==0)
      {
         jbstrcpy(buf,cfgbuf,100,&jbcpos);

         if(areabuf[0] && buf[0] && !args[ARG_NODESC].data)
         {
            strcat(areabuf," ");
            strcat(areabuf,buf);
         }
      }   
   }

   if(areabuf[0])
      printarea(ofh);

   osClose(ofh);
   osClose(ifh);
   osEnd();
   
   exit(OS_EXIT_OK);
}


