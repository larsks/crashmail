#include "crashmail.h"

#ifdef PLATFORM_AMIGA
const uchar ver[]="\0$VER: CrashMailII/" OS_PLATFORM_NAME " " VERSION " " __AMIGADATE__;
#endif

/* Version string to use when writing config */
uchar *config_version="CrashMailII " VERSION;

/*********************************** Global *******************************/

struct jbList PktList;
struct jbList DeleteList;

bool nomem;
bool diskfull;
bool exitprg;

ulong toss_total;
ulong toss_bad;
ulong toss_route;
ulong toss_import;
ulong toss_written;
ulong toss_dupes;

ulong scan_total;
ulong rescan_total;

bool istossing;
bool isscanning;
bool isrescanning;

struct ConfigNode *RescanNode;

ulong DayStatsWritten; /* The area statistics are updated until this day */

struct Config config;

bool ctrlc;

uchar *prinames[]={"Normal","Hold","Normal","Direct","Crash"};

/**************************** Local for this file ****************************/

#define ARG_SCAN            0
#define ARG_TOSS            1
#define ARG_TOSSFILE        2
#define ARG_SCANAREA        3
#define ARG_SCANLIST        4
#define ARG_RESCAN          5
#define ARG_RESCANNODE      6
#define ARG_RESCANMAX       7
#define ARG_SENDQUERY       8
#define ARG_SENDLIST        9
#define ARG_SENDUNLINKED   10
#define ARG_SENDHELP       11
#define ARG_SENDINFO       12
#define ARG_REMOVE         13
#define ARG_SETTINGS       14
#define ARG_VERSION        15
#define ARG_LOCK				16
#define ARG_UNLOCK			17

struct argument args[] =
   { { ARGTYPE_BOOL,   "SCAN",         0    },
     { ARGTYPE_BOOL,   "TOSS",         0    },
     { ARGTYPE_STRING, "TOSSFILE",     NULL },
     { ARGTYPE_STRING, "SCANAREA",     NULL },
     { ARGTYPE_STRING, "SCANLIST",     NULL },
     { ARGTYPE_STRING, "RESCAN",       NULL },
     { ARGTYPE_STRING, "RESCANNODE",   NULL },
     { ARGTYPE_STRING, "RESCANMAX",    NULL },
     { ARGTYPE_STRING, "SENDQUERY",    NULL },
     { ARGTYPE_STRING, "SENDLIST",     NULL },
     { ARGTYPE_STRING, "SENDUNLINKED", NULL },
     { ARGTYPE_STRING, "SENDHELP",     NULL },
     { ARGTYPE_STRING, "SENDINFO",     NULL },
     { ARGTYPE_STRING, "REMOVE",       NULL },
     { ARGTYPE_STRING, "SETTINGS",     NULL },
     { ARGTYPE_BOOL,   "VERSION",      0    },
     { ARGTYPE_BOOL,   "LOCK",         0    },
     { ARGTYPE_BOOL,   "UNLOCK",       0    },
     { ARGTYPE_END,     NULL,          0    } };

bool init_openlog;
bool init_dupebuf;

void Free(void)
{
   if(init_dupebuf)
   {
      FreeDupebuf();
      init_dupebuf=0;
   }

   if(init_openlog)
   {
      CloseLogfile();
      init_openlog=FALSE;
   }

   jbFreeList(&PktList);
   jbFreeList(&DeleteList);
}

bool Init(void)
{
   struct Area *area;

   if(!OpenLogfile(config.cfg_LogFile))
   {
      printf("Failed to open logfile %s\n",config.cfg_LogFile);
      Free();
      return(FALSE);
   }

   init_openlog=TRUE;

   if(config.cfg_DupeMode!=DUPE_IGNORE)
   {
      if(!AllocDupebuf())
      {
         printf("No memory for dupe-check buffer\n");
         Free();
         return(FALSE);
      }

      init_dupebuf=TRUE;

      if(!ReadDupes(config.cfg_DupeFile))
      {
         Free();
         return(FALSE);
      }
   }

   if(!ReadStats(config.cfg_StatsFile))
      return(FALSE);

   nomem=FALSE;
   diskfull=FALSE;
   exitprg=FALSE;

   for(area=(struct Area *)config.AreaList.First;area;area=area->Next)
      if(area->Messagebase) area->Messagebase->active=TRUE;

   return(TRUE);
}

void AfterScanToss(bool success)
{
   struct Area *area;
   struct ConfigNode *cnode;
   uchar errbuf[200];
   ulong day,d,e,i;

   ClosePackets();

   if(success)
   {
      ArchiveOutbound();
      ProcessSafeDelete();
   }

   for(i=0;AvailMessagebases[i].name;i++)
      if(AvailMessagebases[i].active && AvailMessagebases[i].afterfunc)
        (*AvailMessagebases[i].afterfunc)(success);

   if(success)
   {
      /* Rotate last8 if needed */

      if(DayStatsWritten == 0) /* First time we use this statsfile */
         DayStatsWritten = time(NULL) / (24*60*60);

      day=time(NULL) / (24*60*60);

      for(area=(struct Area *)config.AreaList.First;area;area=area->Next)
         if(day > DayStatsWritten)
         {
            for(d=DayStatsWritten;d<day;d++)
            {
               for(e=0;e<7;e++)
                  area->Last8Days[7-e]=area->Last8Days[7-e-1];

               area->Last8Days[0]=0;
            }
         }

      DayStatsWritten=day;

      /* Areas */

      for(area=(struct Area *)config.AreaList.First;area;area=area->Next)
         if(area->NewTexts || area->NewDupes)
         {
            area->Texts+=area->NewTexts;
            area->Last8Days[0]+=area->NewTexts;
            area->Dupes+=area->NewDupes;

            if(area->NewTexts)
               area->LastTime=time(NULL);

            if(area->NewTexts && area->FirstTime==0)
               area->FirstTime=time(NULL);
         }

      for(cnode=(struct ConfigNode *)config.CNodeList.First;cnode;cnode=cnode->Next)
      {
         if(cnode->FirstTime==0 && (cnode->GotEchomails!=0 || cnode->GotNetmails!=0 || cnode->SentEchomails!=0 || cnode->SentNetmails)!=0)
            cnode->FirstTime=time(NULL);
      }

      WriteStats(config.cfg_StatsFile);

      if(config.cfg_DupeMode!=DUPE_IGNORE)
         WriteDupes(config.cfg_DupeFile);

      if(config.changed)
      {
         LogWrite(2,SYSTEMINFO,"Updating configuration file \"%s\"",config.filename);

         if(!UpdateConfig(&config,errbuf))
            LogWrite(1,SYSTEMERR,errbuf);
      }
   }

   if(config.cfg_NodelistType)
      (*config.cfg_NodelistType->nlEnd)();

   jbFreeList(&PktList);
   jbFreeList(&DeleteList);
}

bool BeforeScanToss(void)
{
   struct Area *area;
   struct jbList NewPktFEList;
   struct osFileEntry *fe;
   uchar buf[200];
   int i;

   /* Open nodelist */

   if(config.cfg_NodelistType)
   {
      if(!(*config.cfg_NodelistType->nlStart)(buf))
      {
         LogWrite(1,SYSTEMERR,"%s",buf);
         return(FALSE);
      }
   }

   toss_total=0;
   toss_bad=0;
   toss_route=0;
   toss_import=0;
   toss_dupes=0;
   toss_written=0;

   scan_total=0;

   for(area=(struct Area *)config.AreaList.First;area;area=area->Next)
   {
      area->NewDupes=0;
      area->NewTexts=0;
   }

   jbNewList(&PktList);    /* Created packets */
   jbNewList(&DeleteList); /* For SafeDelete() */

   /* Delete orphan files */

   if(!osReadDir(config.cfg_PacketCreate,&NewPktFEList,IsNewPkt))
   {
      LogWrite(1,SYSTEMERR,"Failed to read directory \"%s\"",config.cfg_PacketCreate);

      if(config.cfg_NodelistType)
         (*config.cfg_NodelistType->nlEnd)();

      return(FALSE);
   }

   for(fe=(struct osFileEntry *)NewPktFEList.First;fe;fe=fe->Next)
   {
      LogWrite(1,SYSTEMINFO,"Deleting orphan tempfile %s",fe->Name);
      MakeFullPath(config.cfg_PacketCreate,fe->Name,buf,200);
      remove(buf);
   }

   jbFreeList(&NewPktFEList);

   for(i=0;AvailMessagebases[i].name;i++)
      if(AvailMessagebases[i].active)
      {
         if(AvailMessagebases[i].beforefunc)
            if(!(*AvailMessagebases[i].beforefunc)())
            {
               if(config.cfg_NodelistType)
                  (*config.cfg_NodelistType->nlEnd)();

               return(FALSE);
            }
      }

   return(TRUE);
}

void Version(void)
{
   int i;

   printf("This is CrashMailII version %s\n",VERSION);
   printf("\n");
   printf("Operating system: %s\n",OS_PLATFORM_NAME);
   printf("Compilation date: %s\n",__DATE__);
   printf("Compilation time: %s\n",__TIME__);
   printf("\n");
   printf("Available messagebase formats:\n");

   for(i=0;AvailMessagebases[i].name;i++)
      printf(" %-10.10s %s\n",AvailMessagebases[i].name,AvailMessagebases[i].desc);

   printf("\n");
   printf("Available nodelist formats:\n");

   for(i=0;AvailNodelists[i].name;i++)
      printf(" %-10.10s %s\n",AvailNodelists[i].name,AvailNodelists[i].desc);
}

bool Rescan(uchar *areaname,uchar *node,ulong max)
{
   struct Area *area;
   struct ConfigNode *cnode;
   struct Node4D n4d;
   bool success;

   if(!Parse4D(node,&n4d))
   {
      LogWrite(1,USERERR,"Invalid node number %s",node);
      return(TRUE);
   }

   for(cnode=(struct ConfigNode *)config.CNodeList.First;cnode;cnode=cnode->Next)
      if(Compare4D(&cnode->Node,&n4d)==0) break;

   if(!cnode)
   {
      LogWrite(1,USERERR,"Unknown node %lu:%lu/%lu.%lu",n4d.Zone,n4d.Net,n4d.Node,n4d.Point);
      return(TRUE);
   }

   for(area=(struct Area *)config.AreaList.First;area;area=area->Next)
      if(!(area->Flags & AREA_BAD) && !(area->Flags & AREA_NETMAIL) && !(area->Flags & AREA_DEFAULT))
         if(stricmp(areaname,area->Tagname)==0) break;

   if(!area)
   {
      LogWrite(1,USERERR,"Unknown area %s",areaname);
      return(TRUE);
   }

   if(!area->Messagebase)
   {
      LogWrite(1,USERERR,"Can't rescan %s, area is pass-through",areaname);
      return(TRUE);
   }

   if(!area->Messagebase->rescanfunc)
   {
      LogWrite(1,USERERR,"Can't rescan %s, messagebase does not support rescan",areaname);
      return(TRUE);
   }

   isrescanning=TRUE;

   if(!BeforeScanToss())
      return(FALSE);

   RescanNode=cnode;
   rescan_total=0;
   success=(*area->Messagebase->rescanfunc)(area,max,HandleRescan);
   RescanNode=NULL;

   if(success)
      LogWrite(4,TOSSINGINFO,"Rescanned %lu messages",rescan_total);

   AfterScanToss(success);

   return(success);
}

bool SendAFList(uchar *node,short type)
{
   struct Node4D n4d;
   struct ConfigNode *cnode;

   if(!BeforeScanToss())
      return(FALSE);

   if(stricmp(node,"ALL")==0)
   {
      for(cnode=(struct ConfigNode *)config.CNodeList.First;cnode;cnode=cnode->Next)
         if(cnode->Flags & NODE_NOTIFY) DoSendAFList(type,cnode);
   }
   else
   {
      if(!Parse4D(node,&n4d))
      {
         LogWrite(1,USERERR,"Invalid node number \"%s\"",node);
         return(FALSE);
      }

      for(cnode=(struct ConfigNode *)config.CNodeList.First;cnode;cnode=cnode->Next)
         if(Compare4D(&cnode->Node,&n4d)==0) break;

      if(cnode)
      {
         DoSendAFList(type,cnode);
      }
      else
      {
         LogWrite(1,USERERR,"Unknown node %lu:%lu/%lu.%lu",n4d.Zone,n4d.Net,n4d.Node,n4d.Point);
         return(FALSE);
      }
   }

   AfterScanToss(TRUE);

   if(nomem || exitprg)
      return(FALSE);

   return(TRUE);
}

bool RemoveArea(uchar *areaname)
{
   struct Area *area;

   if(!BeforeScanToss())
      return(FALSE);

   for(area=(struct Area *)config.AreaList.First;area;area=area->Next)
      if(!(area->Flags & AREA_BAD) && !(area->Flags & AREA_NETMAIL) && !(area->Flags & AREA_DEFAULT))
         if(stricmp(areaname,area->Tagname)==0) break;

   if(!area)
   {
      LogWrite(1,USERERR,"Unknown area %s",areaname);
      return(TRUE);
   }

   LogWrite(1,AREAFIX,"AreaFix: Removing area %s",area->Tagname);
   rawRemoveArea(area);

   AfterScanToss(TRUE);

   return(TRUE);
}

bool done_initconfig;
bool done_osinit;
bool done_welcomemsg;
bool done_init;
bool done_lockconfig;

bool LockConfig(uchar *file)
{
	uchar buf[200];
	osFile fp;
	
	strcpy(buf,file);
	strcat(buf,".busy");

	while(osExists(buf))
	{
		printf("Configuration file %s is already in use, waiting 10 seconds...\n",file);

		osSleep(10);

		if(ctrlc)
			return(FALSE);
	}
	
	if(!(fp=osOpen(buf,MODE_NEWFILE)))
	{
		printf("Failed to create lock file %s\n",buf);
		return(FALSE);
	}
	
	osClose(fp);

	return(TRUE);
}

void UnlockConfig(uchar *file)
{
	uchar buf[200];
	
	strcpy(buf,file);
	strcat(buf,".busy");
	
	remove(buf);
}

void CleanUp(int err)
{
   if(done_welcomemsg)
      LogWrite(2,SYSTEMINFO,"CrashMail end");

	if(done_lockconfig)
		UnlockConfig(config.filename);

   if(done_init)
      Free();

   if(done_initconfig)
      FreeConfig(&config);

   if(done_osinit)
      osEnd();

   exit(err);
}

void breakfunc(int x)
{
   ctrlc=TRUE;
}

int main(int argc, char **argv)
{
   uchar *cfg;
   ulong cfgline;
   short seconderr;
   uchar errorbuf[200];

   signal(SIGINT,breakfunc);

   if(!osInit())
      CleanUp(OS_EXIT_ERROR);

   done_osinit=TRUE;

   if(argc == 2 && strcmp(argv[1],"?")==0)
   {
      printargs(args);
      CleanUp(OS_EXIT_OK);
   }

   if(!parseargs(args,argc,argv))
      CleanUp(OS_EXIT_ERROR);

   if(args[ARG_VERSION].data)
   {
      Version();
      CleanUp(OS_EXIT_OK);
   }

   cfg=OS_CONFIG_NAME;

   if(args[ARG_SETTINGS].data)
      cfg=(uchar *)args[ARG_SETTINGS].data;

   if(args[ARG_LOCK].data)
   {
		if(!LockConfig(cfg))
		{
			printf("Failed to lock configuration file %s\n",cfg);
			CleanUp(OS_EXIT_ERROR);
		}
		
		printf("CrashMail is now locked, use UNLOCK to unlock\n"); 
		CleanUp(OS_EXIT_OK);
	}

   if(args[ARG_UNLOCK].data)
   {
		UnlockConfig(cfg);
		CleanUp(OS_EXIT_OK);
	}

   InitConfig(&config);

   done_initconfig=TRUE;

	if(!(done_lockconfig=LockConfig(cfg)))
	{
		printf("Failed to lock configuration file %s\n",cfg);
		CleanUp(OS_EXIT_ERROR);
	}

   if(!ReadConfig(cfg,&config,&seconderr,&cfgline,errorbuf))
   {
      if(seconderr == READCONFIG_INVALID)
         printf("Configuration error in %s on line %ld:\n%s\n",cfg,cfgline,errorbuf);

      else if(seconderr == READCONFIG_NO_MEM)
         printf("Out of memory\n");

      else
         printf("Failed to read configuration file %s\n",cfg);

      CleanUp(OS_EXIT_ERROR);
   }

   if(!CheckConfig(&config,errorbuf))
   {
      printf("Configuration error in %s:\n%s\n",cfg,errorbuf);
      CleanUp(OS_EXIT_ERROR);
   }

   if(!Init())
      CleanUp(OS_EXIT_ERROR);

   done_init=TRUE;

   LogWrite(2,SYSTEMINFO,"CrashMailII %s started successfully!",VERSION);

   done_welcomemsg=TRUE;

   if(args[ARG_TOSS].data)
      Toss();

   else if(args[ARG_TOSSFILE].data)
      TossFile((uchar *)args[ARG_TOSSFILE].data);

   else if(args[ARG_SCAN].data)
      Scan();

   else if(args[ARG_SCANAREA].data)
      ScanArea((uchar *)args[ARG_SCANAREA].data);

   else if(args[ARG_SCANLIST].data)
      ScanList((uchar *)args[ARG_SCANLIST].data);

   else if(args[ARG_RESCAN].data)
   {
      ulong num;

      num=0;

      if(args[ARG_RESCANMAX].data)
         num=atoi((uchar *)args[ARG_RESCANMAX].data);

      if(!args[ARG_RESCANNODE].data)
         LogWrite(1,USERERR,"No RESCANNODE specified");

      else
         Rescan((uchar *)args[ARG_RESCAN].data,(uchar *)args[ARG_RESCANNODE].data,num);
   }

   else if(args[ARG_SENDLIST].data)
   	SendAFList((uchar *)args[ARG_SENDLIST].data,SENDLIST_FULL);

   else if(args[ARG_SENDQUERY].data)
   	SendAFList((uchar *)args[ARG_SENDQUERY].data,SENDLIST_QUERY);

   else if(args[ARG_SENDUNLINKED].data)
   	SendAFList((uchar *)args[ARG_SENDUNLINKED].data,SENDLIST_UNLINKED);

   else if(args[ARG_SENDHELP].data)
   	SendAFList((uchar *)args[ARG_SENDHELP].data,SENDLIST_HELP);

   else if(args[ARG_SENDINFO].data)
   	SendAFList((uchar *)args[ARG_SENDINFO].data,SENDLIST_INFO);

   else if(args[ARG_REMOVE].data)
   	RemoveArea((uchar *)args[ARG_REMOVE].data);

   if(nomem)
      LogWrite(1,SYSTEMERR,"Out of memory");

   if(diskfull)
      LogWrite(1,SYSTEMERR,"Disk I/O error (disk full?)");

   if(ctrlc)
      LogWrite(1,SYSTEMERR,"*** User break ***");

   CleanUp(OS_EXIT_OK);

/* The next line is never actually executed. It is just there to stop gcc
from giving a warning. */

	return(0);
}
