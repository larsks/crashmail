#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <oslib/os.h>
#include <oslib/osfile.h>

#include <shared/parseargs.h>
#include <shared/node4d.h>

#include <cmnllib/cmnllib.h>

#define VERSION "1.0"

#ifdef PLATFORM_AMIGA
char *ver="$VER: CrashCompileNL " VERSION " " __AMIGADATE__;
#endif

#define ARG_NODE      0
#define ARG_DIRECTORY 1

struct argument args[] =
   { { ARGTYPE_STRING, "NODE",      ARGFLAG_AUTO | ARGFLAG_MANDATORY,  NULL },
     { ARGTYPE_STRING, "DIRECTORY", ARGFLAG_AUTO,                      NULL },
     { ARGTYPE_END,    NULL,        0,                                 0    } };

bool nomem,diskfull;

void strip(char *str)
{                                                                                  int c;

   for(c=strlen(str)-1;str[c] < 33 && c>=0;c--) str[c]=0;
}
                                                                  
int main(int argc, char **argv)
{
	osFile fh;
	char line[200],*dir,*buf;
	struct Node4D n4d;
	struct cmnlIdx idx;
		
   if(!osInit())
      exit(OS_EXIT_ERROR);

   if(argc > 1 &&
	  (strcmp(argv[1],"?")==0      ||
		strcmp(argv[1],"-h")==0     ||
		strcmp(argv[1],"--help")==0 ||
		strcmp(argv[1],"help")==0 ||
		strcmp(argv[1],"/h")==0     ||
		strcmp(argv[1],"/?")==0 ))
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

	if(args[ARG_DIRECTORY].data)
		dir=(char *)args[ARG_DIRECTORY].data;

	else		
		dir=getenv("CMNODELISTDIR");
	
	if(!dir)
	{
		printf("No directory specified and CMNODELISTDIR not set\n");
		osEnd();
		exit(OS_EXIT_ERROR);
	}
	
	if(!Parse4D((char *)args[ARG_NODE].data,&n4d))
	{
		printf("Invalid node %s\n",(char *)args[ARG_NODE].data);
		exit(OS_EXIT_ERROR);
	}

	if(!(fh=cmnlOpenNL(dir)))
	{
		printf("%s\n",cmnlLastError());
		exit(OS_EXIT_ERROR);
	}
	
	idx.zone=n4d.Zone;
	idx.net=n4d.Net;
	idx.node=n4d.Node;
	idx.point=n4d.Point;
	
	if(!cmnlFindNL(fh,dir,&idx,line,200))
	{
		cmnlCloseNL(fh);
		printf("%s\n",cmnlLastError());
		exit(OS_EXIT_ERROR);
	}

	cmnlCloseNL(fh);

	printf("Node %d:%d/%d.%d\n",idx.zone,idx.net,idx.node,idx.point);
	printf("Region %d, Hub %d\n",idx.region,idx.hub);

	strip(line);
	
	if(line[0]==',') 
	{
		buf="Node";
		strtok(line,","); /* Skip number */
	}
	else
	{
		buf=strtok(line,",");
		if(!buf) buf="";
		strtok(NULL,","); /* Skip number */
	}

	printf("Node is listed as a %s\n",buf);
	
	if((buf=strtok(NULL,",")))
	{
		printf("Name: %s\n",buf);
	}

	if((buf=strtok(NULL,",")))
	{
		printf("Location: %s\n",buf);
	}

	if((buf=strtok(NULL,",")))
	{
		printf("Sysop: %s\n",buf);
	}

	if((buf=strtok(NULL,",")))
	{
		printf("Phone: %s\n",buf);
	}

	if((buf=strtok(NULL,",")))
	{
		printf("Baud: %s\n",buf);
	}

	if((buf=strtok(NULL,"")))
	{
		printf("Flags: %s\n",buf);
	}

   osEnd();
   
   exit(OS_EXIT_OK);
}

