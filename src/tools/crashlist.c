#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <ctype.h>

#include <shared/types.h>
#include <shared/jbstrcpy.h>
#include <shared/parseargs.h>
#include <shared/jblist.h>
#include <shared/path.h>
#include <shared/mystrncpy.h>

#include <oslib/os.h>
#include <oslib/osmem.h>
#include <oslib/osfile.h>
#include <oslib/osdir.h>
#include <oslib/osmisc.h>

#define VERSION "1.0"

#ifdef PLATFORM_AMIGA
uchar *ver="$VER: CrashList " VERSION " " __AMIGADATE__;
#endif

#define ARG_DIRECTORY 0

struct argument args[] =
   { { ARGTYPE_STRING, NULL,           NULL },
     { ARGTYPE_END,    NULL,          0    } };

struct idx
{
	ushort zone,net,node,point,region,hub;
	ulong offset;
};

bool nomem,diskfull;

void putuword(uchar *buf,ulong offset,ushort num)
{
   buf[offset]=num%256;
   buf[offset+1]=num/256;
}

void putulong(uchar *buf,ulong offset,ulong num)
{
   buf[offset]=num%256;
   buf[offset+1]=(num / 256) % 256;
   buf[offset+2]=(num / 256 / 256) % 256;
   buf[offset+3]=(num / 256 / 256 / 256) % 256;
}

void WriteIdx(osFile fh,struct idx *idx)
{
	uchar binbuf[16];

	putuword(binbuf,0,idx->zone);
	putuword(binbuf,2,idx->net);
	putuword(binbuf,4,idx->node);
	putuword(binbuf,6,idx->point);
	putuword(binbuf,8,idx->region);
	putuword(binbuf,10,idx->hub);
	putulong(binbuf,12,idx->offset);

	osWrite(fh,binbuf,sizeof(binbuf));
}

uchar nlname[100];
uchar *findfile,*finddir;
time_t newest;

bool isnodelistending(uchar *name)
{
   if(strlen(name)<4)   return(FALSE);

   if(name[strlen(name)-4]!='.') return(FALSE);

   if(!isdigit(name[strlen(name)-3]))   return(FALSE);
   if(!isdigit(name[strlen(name)-2]))   return(FALSE);
   if(!isdigit(name[strlen(name)-1]))   return(FALSE);

   return(TRUE);
}
                                                 
void scandirfunc(uchar *file)
{
	uchar buf[500];
	struct osFileEntry *fe;

	if(isnodelistending(file))
   {
   	if(strnicmp(file,findfile,strlen(file)-4)==0)
      {
			MakeFullPath(finddir,file,buf,500);
			
			if((fe=osGetFileEntry(buf)))
			{
				if(nlname[0]==0 || newest < fe->Date)
				{
					mystrncpy(nlname,fe->Name,100);
					newest=fe->Date;
				}
				
				osFree(fe);
			}
		}
	}
}

bool FindList(uchar *dir,uchar *file,uchar *dest)
{
	MakeFullPath(dir,file,dest,500);
	
	if(osExists(dest))
		return(TRUE);

	nlname[0]=0;
	newest=0;
	findfile=file;
	finddir=dir;
			
   if(!osScanDir(dir,scandirfunc))
   {
      printf("Failed to scan directory %s\n",dir);
      return(FALSE);
   }

	if(nlname[0]==0)
	{
		printf("Found no nodelist matching %s in %s\n",file,dir);
		return(FALSE);
	}
	
	MakeFullPath(dir,nlname,dest,500);

	return(TRUE);
}

void ProcessList(uchar *dir,uchar *file,osFile ifh,ushort defzone)
{
	struct idx idx;
	uchar buf[500];
	osFile nfh;
	
	if(!FindList(dir,file,buf))
		return;
	
	if(!(nfh=osOpen(buf,MODE_OLDFILE)))
	{
		printf("Failed to read %s\n",buf);
		return;
	}

	strcpy(buf,(uchar *)GetFilePart(buf));
	printf("Processing nodelist %s...\n",buf);
	osWrite(ifh,buf,100);

	idx.zone=defzone;
	idx.net=0;
	idx.node=0;
	idx.point=0;
	idx.region=0;
	idx.hub=0;
	idx.offset=0;
	
	idx.offset=osFTell(nfh);	

   while(osFGets(nfh,buf,500))
   {
		if(strnicmp(buf,"Zone,",5)==0)
		{
			idx.zone=atoi(&buf[5]);
			idx.region=0;
			idx.net=idx.zone;
			idx.hub=0;
			idx.node=0;
			idx.point=0;
			WriteIdx(ifh,&idx);
		}
		
		if(strnicmp(buf,"Region,",7)==0)
		{
			idx.region=atoi(&buf[7]);
			idx.net=idx.region;
			idx.hub=0;
			idx.node=0;
			idx.point=0;
			WriteIdx(ifh,&idx);
		}

		if(strnicmp(buf,"Host,",5)==0)
		{
			idx.net=atoi(&buf[5]);
			idx.hub=0;
			idx.node=0;
			idx.point=0;
			WriteIdx(ifh,&idx);
		}

		if(strnicmp(buf,"Hub,",4)==0)
		{
			idx.hub=atoi(&buf[4]);
			idx.node=idx.hub;
			idx.point=0;
			WriteIdx(ifh,&idx);
		}

		if(strnicmp(buf,",",1)==0)
		{
			idx.node=atoi(&buf[1]);
			idx.point=0;
			WriteIdx(ifh,&idx);
		}

		if(strnicmp(buf,"Point,",6)==0)
		{
			idx.point=atoi(&buf[6]);
			WriteIdx(ifh,&idx);
		}

		idx.offset=osFTell(nfh);	
	}

	idx.zone=0;
	idx.region=0;
	idx.net=0;
	idx.hub=0;
	idx.node=0;
	idx.point=0;
	idx.offset=0xffffffff;

	WriteIdx(ifh,&idx);
}

int main(int argc, char **argv)
{
   osFile lfh,ifh;
   uchar *dir,buf[200],cfgbuf[200],file[100];
   ulong jbcpos,zone;  

   if(!osInit())
      exit(OS_EXIT_ERROR);

   if(argc == 2 && strcmp(argv[1],"?")==0)
   {
		printf("Usage: CrashList [<dir>]\n");
      osEnd();
      exit(OS_EXIT_OK);
   }

   if(!parseargs(args,argc,argv))
   {
      osEnd();
      exit(OS_EXIT_ERROR);
   }
   
	dir=OS_CURRENT_DIR;
	
   if(args[ARG_DIRECTORY].data)
		dir=(uchar *)args[ARG_DIRECTORY].data;

	MakeFullPath(dir,"cmnodelist.prefs",buf,200);
		
   if(!(lfh=osOpen(buf,MODE_OLDFILE)))
   {
      printf("Failed to open %s for reading\n",buf);
      osEnd();
      exit(OS_EXIT_ERROR);
   }

	MakeFullPath(dir,"cmnodelist.index",buf,200);

   if(!(ifh=osOpen(buf,MODE_NEWFILE)))
   {
      printf("Failed to open %s for writing (nodelist in use?)\n",buf);
		osClose(lfh);
      osEnd();
      exit(OS_EXIT_ERROR);
   }

 	osWrite(ifh,"CNL1",4);
 
   while(osFGets(lfh,cfgbuf,200))
   {
		if(cfgbuf[0]!=';')
		
      jbcpos=0;

      if(jbstrcpy(file,cfgbuf,100,&jbcpos))
		{
			zone=0;
			
			if(jbstrcpy(buf,cfgbuf,10,&jbcpos))
				zone=atoi(buf);
				
			ProcessList(dir,file,ifh,zone);
		}
   }

	osClose(lfh);
	osClose(ifh);

   osEnd();
   
   exit(OS_EXIT_OK);
}


/* Hitta rätt nodelista */

