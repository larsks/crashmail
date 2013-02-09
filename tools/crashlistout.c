#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>

#include <shared/types.h>
#include <shared/mystrncpy.h>
#include <shared/path.h>
#include <shared/parseargs.h>
#include <shared/jblist.h>
#include <shared/node4d.h>

#include <oslib/os.h>
#include <oslib/osmem.h>
#include <oslib/osfile.h>
#include <oslib/osdir.h>
#include <oslib/osmisc.h>

#define VERSION "1.0"

#define NET(x)  (x >> 16)
#define NODE(x) (x & 0xFFFF)

#define TYPE_CRASH    0
#define TYPE_DIRECT   1
#define TYPE_NORMAL   2
#define TYPE_HOLD     3
#define TYPE_REQUEST  4 

char *type_names[] = { "Crash", "Direct", "Normal", "Hold", "Request" };

uint32_t TotalFiles=0;
uint32_t TotalBytes=0;
uint32_t TotalRequests=0;

struct fileentry
{
   struct fileentry *Next;
   struct Node4D Node;
   char file[100];
   char dir[100];
   uint32_t size;
   time_t date;
   uint32_t type;
   bool flow;
};

struct Node4DPat
{
   char Zone[10];
   char Net[10];
   char Node[10];
   char Point[10];
};

char *cfg_Dir;
uint32_t cfg_Zone;
struct Node4DPat cfg_Pattern;
bool cfg_Verbose;

#define ARG_DIRECTORY  0
#define ARG_ZONE       1
#define ARG_PATTERN    2
#define ARG_VERBOSE    3

struct argument args[] =
   { { ARGTYPE_STRING, "DIRECTORY", ARGFLAG_AUTO,  NULL },
     { ARGTYPE_STRING, "ZONE",      ARGFLAG_AUTO,  NULL },
     { ARGTYPE_STRING, "PATTERN",   ARGFLAG_AUTO,  NULL },
     { ARGTYPE_BOOL,   "VERBOSE",   0,             NULL },
     { ARGTYPE_END,    NULL,        0,             0    } };

struct jbList list;

/* Some stuff for node pattern (taken from CrashMail) */

bool Parse4DPat(char *buf, struct Node4DPat *node)
{
   uint32_t c=0,tempc=0;
   char temp[10];
   bool GotZone=FALSE,GotNet=FALSE,GotNode=FALSE;

   strcpy(node->Zone,"*");
   strcpy(node->Net,"*");
   strcpy(node->Node,"*");
   strcpy(node->Point,"*");

   if(strcmp(buf,"*")==0)
      return(TRUE);

	for(c=0;c<strlen(buf);c++)
	{
		if(buf[c]==':')
		{
         if(GotZone || GotNet || GotNode) return(FALSE);
         strcpy(node->Zone,temp);
         GotZone=TRUE;
			tempc=0;
	   }
		else if(buf[c]=='/')
		{
         if(GotNet || GotNode) return(FALSE);
         strcpy(node->Net,temp);
         GotNet=TRUE;
			tempc=0;
		}
		else if(buf[c]=='.')
		{
         if(GotNode) return(FALSE);
         strcpy(node->Node,temp);
         node->Point[0]=0;
         GotNode=TRUE;
			tempc=0;
		}
		else if((buf[c]>='0' && buf[c]<='9') || buf[c]=='*' || buf[c]=='?')
		{
         if(tempc<9)
         {
            temp[tempc++]=buf[c];
            temp[tempc]=0;
         }
		}
		else return(FALSE);
	}

   if(GotZone && !GotNet)
   {
      strcpy(node->Net,temp);
   }
   else if(GotNode)
   {
      strcpy(node->Point,temp);
   }
   else
   {
      strcpy(node->Node,temp);
      strcpy(node->Point,"0");
   }

   return(TRUE);
}

int NodeCompare(char *pat,uint16_t num)
{
   char buf[10];
   uint8_t c;
   sprintf(buf,"%u",num);

   if(pat[0]==0) return(0);

   for(c=0;c<strlen(pat);c++)
   {
      if(pat[c]=='*') return(0);
      if(pat[c]!=buf[c] && pat[c]!='?') return(1);
   }

   if(buf[c]!=0)
      return(1);

   return(0);
}

int Compare4DPat(struct Node4DPat *nodepat,struct Node4D *node)
{
   if(node->Zone!=0)
      if(NodeCompare(nodepat->Zone, node->Zone )!=0) return(1);

   if(NodeCompare(nodepat->Net,  node->Net  )!=0) return(1);
   if(NodeCompare(nodepat->Node, node->Node )!=0) return(1);
   if(NodeCompare(nodepat->Point,node->Point)!=0) return(1);

   return(0);
}

/* Some other functions from CrashMail */

char *unit(uint32_t i)
{
   static char buf[20];
   if ((i>10000000)||(i<-10000000)) sprintf(buf,"%uM",i/(1024*1024));
   else if ((i>10000)||(i<-10000)) sprintf(buf,"%uK",i/1024);
   else sprintf(buf,"%u",i);
   return buf;
}

unsigned long hextodec(char *hex)
{
   char *hextab="0123456789abcdef";
   int c=0,c2=0;
   unsigned long result=0;

   for(;;)
   {
      for(c2=0;c2<16;c2++)
         if(tolower(hex[c]) == hextab[c2]) break;
		
		if(c2 == 16)
			return(result); /* End of correct hex number */

      result *= 16;
      result += c2;

      c++;
   }
}

void strip(char *str)
{
   int c;

   for(c=strlen(str)-1;str[c] < 33 && c>=0;c--) 
		str[c]=0;
}

/* Display entries in the list */

char *PrintNode(struct Node4D *node)
{
   static char buf[50];

   Print4D(node,buf);

   return(buf);
}

char *PrintDate(time_t date)
{
   static char buf[50];
   struct tm *tp;
   char *monthnames[]={"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec","???"};

   tp=localtime(&date);

   sprintf(buf,"%02d-%s-%02d %02d:%02d",
      tp->tm_mday,
      monthnames[tp->tm_mon],
      tp->tm_year % 100,
      tp->tm_hour,
      tp->tm_min);

   return(buf);
}

char *PrintFlowSize(struct fileentry *fe)
{
   static char buf[50];
   char fullfile[200],line[200];
   osFile os;
   struct osFileEntry *osfe;
   uint32_t files,bytes;

   files=0;
   bytes=0;

   MakeFullPath(cfg_Dir,fe->file,fullfile,200);

   if(!(os=osOpen(fullfile,MODE_OLDFILE)))
   {
      sprintf(buf,"?/?");
      return(buf);
   }

   while(osFGets(os,line,200))
   {
      strip(line);

      if(line[0])
      {
         if(line[0] == '#' || line[0] == '^' || line[0] == '-')
            strcpy(line,&line[1]);
      
         if(stricmp(GetFilePart(line),line) == 0)
         {
            /* No path specified */

            MakeFullPath(fe->dir,line,fullfile,200);
            osfe=osGetFileEntry(fullfile);
         }
         else
         {
            osfe=osGetFileEntry(line);
         }

         if(osfe)
         {
            files++;
            bytes+=osfe->Size;

            TotalFiles++;
            TotalBytes+=osfe->Size;

            osFree(osfe);
         }
      }
   }

   osClose(os);

   sprintf(buf,"%s/%u",unit(bytes),files);
   
   return(buf);
}

void DisplayFlowContents(struct fileentry *fe)
{
   char size[40],*todo;
   char fullfile[200],line[200];
   osFile os;
   struct osFileEntry *osfe;

   MakeFullPath(cfg_Dir,fe->file,fullfile,200);

   if(!(os=osOpen(fullfile,MODE_OLDFILE)))
   {
      printf("Failed to open file\n");
   }
   else
   {
      while(osFGets(os,line,200))
      {
         strip(line);

         if(line[0])
         {
            todo="";

            if(line[0] == '#' || line[0] == '^') todo="(To be truncated)";
            if(line[0] == '-')                   todo="(To be deleted)";

            if(line[0] == '#' || line[0] == '^' || line[0] == '-')
               strcpy(line,&line[1]);
      
            if(stricmp(GetFilePart(line),line) == 0)
            {
               /* No path specified */

               MakeFullPath(fe->dir,line,fullfile,200);
               osfe=osGetFileEntry(fullfile);
            }
            else
            {
               osfe=osGetFileEntry(line);
            }

            strcpy(size,"Not found");

            if(osfe)
            {
               sprintf(size, "%s", unit(osfe->Size));
               osFree(osfe);
            }

            printf(" %-39.39s %10s   %s\n",line,size,todo);
         }
      }

      osClose(os);
   }

   printf("\n");
}

char *PrintReqNums(struct fileentry *fe)
{
   static char buf[50];
   char fullfile[200],line[200];
   osFile os;
   uint32_t reqs;

   reqs=0;

   MakeFullPath(cfg_Dir,fe->file,fullfile,200);

   if(!(os=osOpen(fullfile,MODE_OLDFILE)))
   {
      sprintf(buf,"?/?");
      return(buf);
   }

   while(osFGets(os,line,200))
   {
      strip(line);

      if(line[0])
      {
         reqs++;
         TotalRequests++;
      }
   }

   sprintf(buf,"-/%u",reqs);
   
   return(buf);
}

void DisplayReqContents(struct fileentry *fe)
{
   char fullfile[200],line[200];
   osFile os;

   MakeFullPath(cfg_Dir,fe->file,fullfile,200);

   if(!(os=osOpen(fullfile,MODE_OLDFILE)))
   {
      printf("Failed to open file\n");
   }
   else
   {
      while(osFGets(os,line,200))
      {
         strip(line);

         if(line[0])
            printf(" %s\n",line);
      }

      osClose(os);
   }

   printf("\n");
}

void DisplayFlow(struct fileentry *fe)
{
   printf("%-8.8s %-17.17s %-16.16s %7.7s   %s\n",
      type_names[fe->type],PrintNode(&fe->Node),PrintDate(fe->date),PrintFlowSize(fe),fe->file);

   if(cfg_Verbose)
      DisplayFlowContents(fe);
}

void DisplayPkt(struct fileentry *fe)
{
   char buf[100];

   sprintf(buf,"%s/1",unit(fe->size));

   printf("%-8.8s %-17.17s %-16.16s %7.7s   %s\n",
      type_names[fe->type],PrintNode(&fe->Node),PrintDate(fe->date),buf,fe->file);

   if(cfg_Verbose)
      printf("\n");
}

void DisplayReq(struct fileentry *fe)
{
   printf("%-8.8s %-17.17s %-16.16s %7.7s   %s\n",
      type_names[fe->type],PrintNode(&fe->Node),PrintDate(fe->date),PrintReqNums(fe),fe->file);

   if(cfg_Verbose)
      DisplayReqContents(fe);
}

int sortcompare(const void *f1, const void *f2)
{
   struct fileentry *e1,*e2;

   e1=*(struct fileentry **)f1;
   e2=*(struct fileentry **)f2;

   return Compare4D(&e1->Node,&e2->Node);
}

void sortlist(struct jbList *list)
{
   struct jbNode *jb,**buf,**work;
   uint32_t count=0;

   for(jb=list->First;jb;jb=jb->Next)
      count++;

   if(count < 2)
      return;

   if(!(buf=(struct jbNode **)osAlloc(count * sizeof(struct jbNode *))))
      return;

   work=buf;

   for(jb=list->First;jb;jb=jb->Next)
      *work++=jb;

   qsort(buf,(size_t)count,(size_t)sizeof(struct jbNode *),sortcompare);

   jbNewList(list);

   for(work=buf;count--;)
      jbAddNode(list,*work++);

   osFree(buf);
}

void addentry(char *dir,char *file,uint32_t type,struct Node4D *boss,bool flow)
{
   struct osFileEntry *fe;
   struct fileentry *entry;
   struct Node4D n4d;
   char buf[200];
   char buf2[200];
   uint32_t hex;

   hex=hextodec(file);

   if(boss)
   {
      Copy4D(&n4d,boss);
      n4d.Point = hex;
   }
   else
   {
      n4d.Zone = cfg_Zone;
      n4d.Net = NET(hex);
      n4d.Node = NODE(hex);
      n4d.Point = 0;
   }

   if(Compare4DPat(&cfg_Pattern,&n4d)!=0)
      return;

   if(dir) MakeFullPath(dir,file,buf,200);
   else    mystrncpy(buf,file,200);

   MakeFullPath(cfg_Dir,buf,buf2,200);

   if(!(fe=osGetFileEntry(buf2)))
   {
      return;
   }

   if(!(entry=osAlloc(sizeof(struct fileentry))))
   {
      osFree(fe);
      return;
   }

   Copy4D(&entry->Node,&n4d);

   if(dir)
   {
      MakeFullPath(dir,file,entry->file,100);
      MakeFullPath(cfg_Dir,dir,entry->dir,100);
   }
   else
   {
      mystrncpy(entry->file,file,100);
      mystrncpy(entry->dir,cfg_Dir,100);
   }

   mystrncpy(entry->file,buf,100);
   entry->size=fe->Size;
   entry->date=fe->Date;
   entry->type=type;
   entry->flow=flow;

   jbAddNode(&list,(struct jbNode *)entry);
   osFree(fe);
}

struct Node4D *scandir_boss;
char *scandir_dir;

void scandirfunc(char *file)
{
   char *extptr;
   uint32_t hex;

   if(strlen(file) != 12)
      return;

   extptr=&file[strlen(file)-4];

   if(stricmp(extptr,".PNT")==0 && !scandir_boss)
   {
      char buf[200];
      struct Node4D n4d;

      hex=hextodec(file);

      n4d.Zone = cfg_Zone;
      n4d.Net = NET(hex);
      n4d.Node = NODE(hex);
      n4d.Point = 0;

      scandir_dir = file;
      scandir_boss = &n4d;

      MakeFullPath(cfg_Dir,file,buf,200);
      osScanDir(buf,scandirfunc);

      scandir_dir = NULL;
      scandir_boss = NULL;
   }

   if(!stricmp(extptr,".REQ")) addentry(scandir_dir,file,TYPE_REQUEST,scandir_boss,TRUE);

   if(!stricmp(extptr,".CLO")) addentry(scandir_dir,file,TYPE_CRASH,scandir_boss,TRUE);
   if(!stricmp(extptr,".DLO")) addentry(scandir_dir,file,TYPE_DIRECT,scandir_boss,TRUE);
   if(!stricmp(extptr,".FLO")) addentry(scandir_dir,file,TYPE_NORMAL,scandir_boss,TRUE);
   if(!stricmp(extptr,".HLO")) addentry(scandir_dir,file,TYPE_HOLD,scandir_boss,TRUE);

   if(!stricmp(extptr,".CUT")) addentry(scandir_dir,file,TYPE_CRASH,scandir_boss,FALSE);
   if(!stricmp(extptr,".DUT")) addentry(scandir_dir,file,TYPE_DIRECT,scandir_boss,FALSE);
   if(!stricmp(extptr,".OUT")) addentry(scandir_dir,file,TYPE_NORMAL,scandir_boss,FALSE);
   if(!stricmp(extptr,".HUT")) addentry(scandir_dir,file,TYPE_HOLD,scandir_boss,FALSE);
}

int main(int argc, char **argv)
{
   char *var;
   struct fileentry *fe;

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

   jbNewList(&list);

   /* get outbound dir */

   if((var=getenv("CMOUTBOUND")))
      cfg_Dir=var;

   else
      cfg_Dir=OS_CURRENT_DIR;

   if(args[ARG_DIRECTORY].data)
      cfg_Dir=(char *)args[ARG_DIRECTORY].data;

   /* Get zone */

   if((var=getenv("CMOUTBOUNDZONE")))
      cfg_Zone=atoi(var);

   else
      cfg_Zone=2;

   if(args[ARG_ZONE].data)
      cfg_Zone=atoi((char *)args[ARG_ZONE].data);

   /* Get pattern */

   strcpy(cfg_Pattern.Zone,"*");
   strcpy(cfg_Pattern.Net,"*");
   strcpy(cfg_Pattern.Node,"*");
   strcpy(cfg_Pattern.Point,"*");

   if(args[ARG_PATTERN].data)
   {
      if(!Parse4DPat((char *)args[ARG_PATTERN].data,&cfg_Pattern))
      {
         printf("Invalid node pattern \"%s\"\n",(char *)args[ARG_PATTERN].data);
         osEnd();
         exit(OS_EXIT_ERROR);
      }
   }

   /* Get verbose flag */

   cfg_Verbose=FALSE;

   if(args[ARG_VERBOSE].data)
      cfg_Verbose=TRUE;

   /* Real program starts here */

   printf("CrashListOut " VERSION "\n\n");

   scandir_dir = NULL;
   scandir_boss = NULL;

   if(!osScanDir(cfg_Dir,scandirfunc))
   {
		uint32_t err=osError();
      printf("Failed to scan directory %s\n",cfg_Dir);
		printf("Error: %s",osErrorMsg(err));		
      return(FALSE);
   }

   sortlist(&list);

   printf("%-8.8s %-17.17s %-16.16s %7.7s   %s\n\n",
      "Type","Node","Last Change","B/F","File");

   if(list.First)
   {
      for(fe=(struct fileentry *)list.First;fe;fe=fe->Next)
      {
         if(fe->type == TYPE_REQUEST) DisplayReq(fe);
         else if(fe->flow)            DisplayFlow(fe);
         else                         DisplayPkt(fe);
      }

      if(!cfg_Verbose) printf("\n");
      printf("Totally %s bytes in %u files to send, %u requests.\n",unit(TotalBytes),TotalFiles,TotalRequests);
   }
   else
   {
      printf("Outbound directory is empty.\n");
   }

   jbFreeList(&list);

   exit(OS_EXIT_OK);
}


