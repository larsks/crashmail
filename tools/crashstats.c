#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <signal.h>
#include <ctype.h>

#include <shared/types.h>
#include <shared/jblist.h>
#include <shared/parseargs.h>
#include <shared/node4d.h>

#include <oslib/os.h>
#include <oslib/osmem.h>
#include <oslib/osfile.h>
#include <oslib/osmisc.h>

#define VERSION "1.0"
#define COPYRIGHT "1998"

#ifdef PLATFORM_AMIGA
uchar *ver="$VER: CrashStats "VERSION" ("__COMMODORE_DATE__")";
#endif

#define STATS_IDENTIFIER   "CST3"

static size_t ptrsize = sizeof(void *);

struct DiskAreaStats
{
   uchar Tagname[80];
   struct Node4D Aka;

   uchar Group;
   uchar fill_to_make_even; /* Just ignore this one */

   ulong TotalTexts;
   ushort Last8Days[8];
   ulong Dupes;

   time_t FirstTime;
   time_t LastTime;
};

struct DiskNodeStats
{
   struct Node4D Node;
   ulong GotNetmails;
   ulong GotNetmailBytes;
   ulong SentNetmails;
   ulong SentNetmailBytes;
   ulong GotEchomails;
   ulong GotEchomailBytes;
   ulong SentEchomails;
   ulong SentEchomailBytes;
   ulong Dupes;
   time_t FirstTime;
};

struct StatsNode
{
   struct StatsNode *Next;
   uchar Tagname[80];
   ulong Average;
   ulong Total;
   ulong Dupes;
   time_t FirstTime;
   time_t LastTime;
   ushort Last8Days[8];
};

struct NodeStatsNode
{
   struct NodeStatsNode *Next;
   struct Node4D Node;
   ulong GotNetmails;
   ulong GotNetmailBytes;
   ulong SentNetmails;
   ulong SentNetmailBytes;
   ulong GotEchomails;
   ulong GotEchomailBytes;
   ulong SentEchomails;
   ulong SentEchomailBytes;
   ulong Dupes;
   ulong Days;
   time_t FirstTime;
};

#define ARG_FILE    0
#define ARG_SORT    1
#define ARG_LAST7   2
#define ARG_NOAREAS 3
#define ARG_NONODES 4
#define ARG_GROUP   5

struct argument args[] =
   { { ARGTYPE_STRING, "FILE",    ARGFLAG_AUTO | ARGFLAG_MANDATORY, NULL },
     { ARGTYPE_STRING, "SORT",    0,                                NULL },
     { ARGTYPE_BOOL,   "LAST7",   0,                                NULL },
     { ARGTYPE_BOOL,   "NOAREAS", 0,                                NULL },
     { ARGTYPE_BOOL,   "NONODES", 0,                                NULL },
     { ARGTYPE_STRING, "GROUP",   0,                                NULL },
     { ARGTYPE_END,     NULL,     0,                                0    } };

bool diskfull;

int CompareAlpha(const void *a1,const void *a2)
{
   struct StatsNode **s1,**s2;
   
   s1=(struct StatsNode **)a1;
   s2=(struct StatsNode **)a2;

   return(stricmp((*s1)->Tagname,(*s2)->Tagname));
}

int CompareTotal(const void *a1,const void *a2)
{
   struct StatsNode **s1,**s2;
   
   s1=(struct StatsNode **)a1;
   s2=(struct StatsNode **)a2;

   if((*s1)->Total < (*s2)->Total) return(1);
   if((*s1)->Total > (*s2)->Total) return(-1);
   return(0);
}

int CompareDupes(const void *a1,const void *a2)
{
   struct StatsNode **s1,**s2;
   
   s1=(struct StatsNode **)a1;
   s2=(struct StatsNode **)a2;

   if((*s1)->Dupes < (*s2)->Dupes) return(1);
   if((*s1)->Dupes > (*s2)->Dupes) return(-1);
   return(0);
}

int CompareMsgsDay(const void *a1,const void *a2)
{
   struct StatsNode **s1,**s2;
   
   s1=(struct StatsNode **)a1;
   s2=(struct StatsNode **)a2;

   if((*s1)->Average < (*s2)->Average) return(1);
   if((*s1)->Average > (*s2)->Average) return(-1);
   return(0);
}

int CompareFirstTime(const void *a1,const void *a2)
{
   struct StatsNode **s1,**s2;
   
   s1=(struct StatsNode **)a1;
   s2=(struct StatsNode **)a2;

   if((*s1)->FirstTime < (*s2)->FirstTime) return(1);
   if((*s1)->FirstTime > (*s2)->FirstTime) return(-1);
   return(0);
}

int CompareLastTime(const void *a1,const void *a2)
{
   struct StatsNode **s1,**s2;
   
   s1=(struct StatsNode **)a1;
   s2=(struct StatsNode **)a2;

   if((*s1)->LastTime < (*s2)->LastTime) return(1);
   if((*s1)->LastTime > (*s2)->LastTime) return(-1);
   return(0);
}

bool Sort(struct jbList *list,uchar sortmode)
{
   ulong nc;
   struct StatsNode *sn,**buf,**work;

   nc=0;

   for(sn=(struct StatsNode *)list->First;sn;sn=sn->Next)
      nc++;

   if(nc==0)
      return(TRUE); /* Nothing to sort */

   if(!(buf=(struct StatsNode **)osAlloc(nc*sizeof(struct StatsNode *))))
      return(FALSE);

   work=buf;

   for(sn=(struct StatsNode *)list->First;sn;sn=sn->Next)
      *work++=sn;

   switch(sortmode)
   {
      case 'a': qsort(buf,nc,ptrsize,CompareAlpha);
                break;

      case 't': qsort(buf,nc,ptrsize,CompareTotal);
                break;

      case 'm': qsort(buf,nc,ptrsize,CompareMsgsDay);
                break;

      case 'd': qsort(buf,nc,ptrsize,CompareFirstTime);
                break;

      case 'l': qsort(buf,nc,ptrsize,CompareLastTime);
                break;

      case 'u': qsort(buf,nc,ptrsize,CompareDupes);
                break;
   }

   jbNewList(list);

   for(work=buf;nc--;)
      jbAddNode(list,(struct jbNode *)*work++);

   osFree(buf);

	return(TRUE);
}

int CompareNodes(const void *a1,const void *a2)
{
   struct NodeStatsNode **s1,**s2;
   
   s1=(struct NodeStatsNode **)a1;
   s2=(struct NodeStatsNode **)a2;

   return(Compare4D(&(*s1)->Node,&(*s2)->Node));
}

bool SortNodes(struct jbList *list)
{
   struct NodeStatsNode *sn,**buf,**work;
   ulong nc;
   
   nc=0;

   for(sn=(struct NodeStatsNode *)list->First;sn;sn=sn->Next)
      nc++;

   if(nc==0)
      return(TRUE); /* Nothing to sort */

   if(!(buf=(struct NodeStatsNode **)osAlloc(nc*sizeof(struct NodeStatsNode *))))
      return(FALSE);

   work=buf;

   for(sn=(struct NodeStatsNode *)list->First;sn;sn=sn->Next)
      *work++=sn;

   qsort(buf,nc,ptrsize,CompareNodes);

   jbNewList(list);

   for(work=buf;nc--;)
      jbAddNode(list,(struct jbNode *)*work++);

   osFree(buf);

	return(TRUE);
}

char *unit(long i)
{
   static char buf[40];
   if ((i>10000000)||(i<-10000000)) sprintf(buf,"%ld MB",i/(1024*1024));
   else if ((i>10000)||(i<-10000)) sprintf(buf,"%ld KB",i/1024);
   else sprintf(buf,"%ld bytes",i);
   return buf;
}

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

ulong CalculateAverage(ushort *last8array,ulong total,ulong daystatswritten,time_t firstday)
{
   ushort days,c;
   ulong sum;

   if(daystatswritten == 0 || firstday == 0)
      return(0);

   days=daystatswritten-firstday;
   if(days > 7) days=7;

   sum=0;

   for(c=1;c<days+1;c++)
      sum+=last8array[c];

   if(days == 0) days=1;

   if(sum == 0 && total!=0)
   {
      days=daystatswritten-firstday;
      if(days==0) days=1;

      return(total/days);
   }

   return(sum/days);
}

bool ctrlc;

void breakfunc(int x)
{
   ctrlc=TRUE;
}

int main(int argc, char **argv)
{
   osFile fh;
   ulong total,areas,totaldupes;
   time_t firsttime,t;
   ulong DayStatsWritten;
   uchar buf[200],date[30],date2[30];
   struct DiskAreaStats dastat;
   struct DiskNodeStats dnstat;
   struct StatsNode *sn;
   struct NodeStatsNode *nsn;
   struct jbList StatsList;
   struct jbList NodesList;
   ulong c,num,tot;
   ushort total8days[8];
   uchar sortmode;
   struct tm *tp;
   uchar *monthnames[]={"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec","???"};

   signal(SIGINT,breakfunc);
         
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

   sortmode='a';
   
   if(args[ARG_SORT].data)
      sortmode=tolower(((uchar *)args[ARG_SORT].data)[0]);
      
   if(!strchr("amtdlu",sortmode))
   {
      printf("Unknown sort mode %c\n",sortmode);
      osEnd();
      exit(OS_EXIT_ERROR);
   }

   if(args[ARG_NOAREAS].data && args[ARG_NONODES].data)
   {
      printf("Nothing to do\n");
      osEnd();
      exit(OS_EXIT_ERROR);
   }

   printf("CrashStats "VERSION" © "  COPYRIGHT " Johan Billing\n");

   if(!(fh=osOpen(args[ARG_FILE].data,MODE_OLDFILE)))
   {
		ulong err=osError();
      printf("Error opening %s\n",(char *)args[ARG_FILE].data);
		printf("Error: %s\n",osErrorMsg(err));		
      osEnd();
      exit(OS_EXIT_ERROR);
   }

   osRead(fh,buf,4);
   buf[4]=0;

   if(strcmp(buf,STATS_IDENTIFIER)!=0)
   {
      printf("Unknown format of stats file\n");
      osClose(fh);
      osEnd();
      exit(OS_EXIT_ERROR);
   }

   osRead(fh,&DayStatsWritten,sizeof(ulong));

   total=0;
   totaldupes=0;
   firsttime=0;
   areas=0;

   for(c=0;c<8;c++)
   	total8days[c]=0;

   jbNewList(&StatsList);
   jbNewList(&NodesList);

   osRead(fh,&num,sizeof(ulong));
   c=0;

   if(!args[ARG_NOAREAS].data)
   {
      while(c<num && osRead(fh,&dastat,sizeof(struct DiskAreaStats))==sizeof(struct DiskAreaStats))
      {
         if(!args[ARG_GROUP].data || CheckFlags(dastat.Group,args[ARG_GROUP].data))
         {
            if(!(sn=osAlloc(sizeof(struct StatsNode))))
            {
               printf("Out of memory\n");
               jbFreeList(&StatsList);
               osClose(fh);
               osEnd();
               exit(OS_EXIT_ERROR);
            }

            jbAddNode(&StatsList,(struct jbNode *)sn);

            strcpy(sn->Tagname,dastat.Tagname);
            sn->Dupes=dastat.Dupes;
            sn->Total=dastat.TotalTexts;
            sn->FirstTime=dastat.FirstTime;
            sn->LastTime=dastat.LastTime;
            memcpy(&sn->Last8Days[0],&dastat.Last8Days[0],8*sizeof(ushort));

            sn->Average=CalculateAverage(&dastat.Last8Days[0],dastat.TotalTexts,DayStatsWritten,sn->FirstTime / (24*60*60));
         }

         if(dastat.FirstTime!=0)
            if(firsttime==0 || firsttime > dastat.FirstTime)
               firsttime=dastat.FirstTime;

         c++;
      }
   }
   else
   {
      while(c<num && osRead(fh,&dastat,sizeof(struct DiskAreaStats))==sizeof(struct DiskAreaStats))
         c++;
   }

   osRead(fh,&num,sizeof(ulong));
   c=0;

   if(!args[ARG_NONODES].data)
   {
      while(c<num && osRead(fh,&dnstat,sizeof(struct DiskNodeStats))==sizeof(struct DiskNodeStats))
      {
         if(!(nsn=osAlloc(sizeof(struct NodeStatsNode))))
         {
            printf("Out of memory\n");
            jbFreeList(&NodesList);
            jbFreeList(&StatsList);
            osClose(fh);
            osEnd();
            exit(OS_EXIT_ERROR);
         }

         jbAddNode(&NodesList,(struct jbNode *)nsn);

         Copy4D(&nsn->Node,&dnstat.Node);

         nsn->GotNetmails=dnstat.GotNetmails;
         nsn->GotNetmailBytes=dnstat.GotNetmailBytes;
         nsn->SentNetmails=dnstat.SentNetmails;
         nsn->SentNetmailBytes=dnstat.SentNetmailBytes;
         nsn->GotEchomails=dnstat.GotEchomails;
         nsn->GotEchomailBytes=dnstat.GotEchomailBytes;
         nsn->SentEchomails=dnstat.SentEchomails;
         nsn->SentEchomailBytes=dnstat.SentEchomailBytes;
         nsn->Dupes=dnstat.Dupes;

         nsn->Days=DayStatsWritten-dnstat.FirstTime % (24*60*60);
         if(nsn->Days==0) nsn->Days=1;

         nsn->FirstTime=dnstat.FirstTime;

         if(dnstat.FirstTime!=0)
            if(firsttime==0 || firsttime > dnstat.FirstTime)
               firsttime=dnstat.FirstTime;

         c++;
      }
   }
   else
   {
      while(c<num && osRead(fh,&dnstat,sizeof(struct DiskNodeStats))==sizeof(struct DiskNodeStats))
         c++;
   }

   osClose(fh);

   t=(time_t)DayStatsWritten * 24*60*60;

   tp=localtime(&firsttime);
   sprintf(date,"%02d-%s-%02d",tp->tm_mday,monthnames[tp->tm_mon],tp->tm_year%100);
   
   tp=localtime(&t);
   sprintf(date2,"%02d-%s-%02d",tp->tm_mday,monthnames[tp->tm_mon],tp->tm_year%100);

   printf("\nStatistics from %s to %s\n",date,date2);
   
   if(!ctrlc && !args[ARG_NOAREAS].data)
   {
      Sort(&StatsList,'a');
      Sort(&StatsList,sortmode);
      printf("\n");

      if(args[ARG_LAST7].data)
      {
         printf("Area                             ");

         for(c=1;c<8;c++)
         {
            t=(DayStatsWritten-c)*24*60*60;
            tp=localtime(&t);
            printf("   %02d",tp->tm_mday);
         }

         printf("   Total\n============================================================================\n");

         if(!ctrlc)
         {
            for(sn=(struct StatsNode *)StatsList.First;sn && !ctrlc;sn=sn->Next)
            {
               tot=0;

               for(c=1;c<8;c++)
                  tot+=sn->Last8Days[c];

               printf("%-33.33s %4d %4d %4d %4d %4d %4d %4d : %5ld\n",
                  sn->Tagname,
                  sn->Last8Days[1],
                  sn->Last8Days[2],
                  sn->Last8Days[3],
                  sn->Last8Days[4],
                  sn->Last8Days[5],
                  sn->Last8Days[6],
                  sn->Last8Days[7],
                  tot);

               for(c=1;c<8;c++)
                  total8days[c]+=sn->Last8Days[c];

               areas++;
            }

            if(!ctrlc)
            {
               tot=0;

               for(c=1;c<8;c++)
                  tot+=total8days[c];

               printf("=============================================================================\n");
               sprintf(buf,"Totally in all %lu areas",areas);

               printf("%-33.33s %4d %4d %4d %4d %4d %4d %4d : %5ld\n",
                  buf,
                  total8days[1],
                  total8days[2],
                  total8days[3],
                  total8days[4],
                  total8days[5],
                  total8days[6],
                  total8days[7],
                  tot);
            }
         }
      }
      else
      {
         printf("Area                           First     Last         Msgs  Msgs/day   Dupes\n");
         printf("============================================================================\n");

         if(!ctrlc)
         {
            for(sn=(struct StatsNode *)StatsList.First;sn && !ctrlc;sn=sn->Next)
            {
               if(sn->LastTime==0)
               {
                  strcpy(date2,"<Never>");
               }
               else
               {
                  tp=localtime(&sn->LastTime);
                  sprintf(date2,"%02d-%s-%02d",tp->tm_mday,monthnames[tp->tm_mon],tp->tm_year%100);
               }

               if(sn->FirstTime==0)
               {
                  strcpy(date,"<Never>");
               }
               else
               {
                  tp=localtime(&sn->FirstTime);
                  sprintf(date,"%02d-%s-%02d",tp->tm_mday,monthnames[tp->tm_mon],tp->tm_year%100);
               }

               for(c=0;c<8;c++)
                  total8days[c]+=sn->Last8Days[c];

               total+=sn->Total;
               totaldupes+=sn->Dupes;
               areas++;

               printf("%-30.30s %-9.9s %-9.9s %7ld   %7ld %7ld\n",sn->Tagname,date,date2,sn->Total,sn->Average,sn->Dupes);
            }
         }

         if(!ctrlc)
         {
            printf("============================================================================\n");
            sprintf(buf,"Totally in all %lu areas",areas);
            printf("%-42s         %7ld   %7ld %7ld\n",
               buf,
               total,
               CalculateAverage(&total8days[0],total,DayStatsWritten,firsttime / (24*60*60)),
               totaldupes);
         }
      }
   }

   if(!ctrlc && !args[ARG_NONODES].data)
   {
      SortNodes(&NodesList);

      printf("\n");
      printf("Nodes statistics\n");
      printf("================\n");

      for(nsn=(struct NodeStatsNode *)NodesList.First;nsn && !ctrlc;nsn=nsn->Next)
      {
         if(nsn->FirstTime==0)
         {
            strcpy(date,"<Never>");
         }
         else
         {
            tp=localtime(&nsn->FirstTime);
            sprintf(date,"%0d-%s-%0d",tp->tm_mday,monthnames[tp->tm_mon],tp->tm_year%100);
         }

         sprintf(buf,"%u:%u/%u.%u",nsn->Node.Zone,nsn->Node.Net,nsn->Node.Node,nsn->Node.Point);

			printf("%-30.40s Statistics since: %s\n\n",buf,date);
			printf("                                  Sent netmails: %lu/%s\n",nsn->SentNetmails,unit(nsn->SentNetmailBytes));
			printf("                              Received netmails: %lu/%s\n",nsn->GotNetmails,unit(nsn->GotNetmailBytes));
			printf("                                 Sent echomails: %lu/%s\n",nsn->SentEchomails,unit(nsn->SentEchomailBytes));
			printf("                             Received echomails: %lu/%s\n",nsn->GotEchomails,unit(nsn->GotEchomailBytes));
			printf("                                          Dupes: %lu\n",nsn->Dupes);
         printf("\n");
      }
   }

   if(ctrlc)
   {
      printf("*** Break\n");
   }
   else
   {
      printf("\n");
   }

   jbFreeList(&StatsList);
   jbFreeList(&NodesList);

   osEnd();
   
   exit(OS_EXIT_OK);
}






