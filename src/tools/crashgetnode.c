#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <oslib/os.h>
#include <oslib/osfile.h>

#include <shared/parseargs.h>
#include <cmnllib/cmnllib.h>

#define VERSION "1.0"

#ifdef PLATFORM_AMIGA
uchar *ver="$VER: CrashCompileNL " VERSION " " __AMIGADATE__;
#endif

struct Node4D
{
   ushort Zone,Net,Node,Point;
};

bool nomem,diskfull;

bool Parse4D(uchar *buf, struct Node4D *node)
{
   ulong c=0,val=0;
   bool GotZone=FALSE,GotNet=FALSE,GotNode=FALSE;

   node->Zone=0;
   node->Net=0;
   node->Node=0;
   node->Point=0;

	for(c=0;c<strlen(buf);c++)
	{
		if(buf[c]==':')
		{
         if(GotZone || GotNet || GotNode) return(FALSE);
			node->Zone=val;
         GotZone=TRUE;
			val=0;
	   }
		else if(buf[c]=='/')
		{
         if(GotNet || GotNode) return(FALSE);
         node->Net=val;
         GotNet=TRUE;
			val=0;
		}
		else if(buf[c]=='.')
		{
         if(GotNode) return(FALSE);
         node->Node=val;
         GotNode=TRUE;
			val=0;
		}
		else if(buf[c]>='0' && buf[c]<='9')
		{
         val*=10;
         val+=buf[c]-'0';
		}
		else return(FALSE);
	}
   if(GotZone && !GotNet)  node->Net=val;
   else if(GotNode)        node->Point=val;
   else                    node->Node=val;

   return(TRUE);
}

void strip(uchar *str)
{                                                                                  int c;

   for(c=strlen(str)-1;str[c] < 33 && c>=0;c--) str[c]=0;
}
                                                                  
int main(int argc, char **argv)
{
	osFile fh;
	uchar line[200],*dir,*buf;
	struct Node4D n4d;
	struct cmnlIdx idx;
		
   if(!osInit())
      exit(OS_EXIT_ERROR);

   if(argc != 2 && argc!=3)
   {
		printf("Usage: CrashGetNode <node> [<nodelistdir>]\n");
      osEnd();
      exit(OS_EXIT_OK);
   }
	
	if(argc == 3)
		dir=argv[2];

	else		
		dir=getenv("CMNODELISTDIR");
	
	if(!dir)
	{
		printf("No directory specified and CMNODELISTDIR not set\n");
		osEnd();
		exit(OS_EXIT_ERROR);
	}
	
	if(!Parse4D(argv[1],&n4d))
	{
		printf("Invalid node %s\n",argv[1]);
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

