#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>

#include <shared/parseargs.h>
#include <shared/jblist.h>
#include <shared/jbstrcpy.h>
#include <shared/path.h>

#include <oslib/os.h>
#include <oslib/osmem.h>
#include <oslib/osfile.h>
#include <oslib/osdir.h>
#include <oslib/ospattern.h>

#include <shared/storedmsg.h>
#include <shared/fidonet.h>

#include <jamlib/jam.h>

#define VERSION "1.0"

#ifdef PLATFORM_AMIGA
uchar *ver="$VER: CrashMaint "VERSION" ("__COMMODORE_DATE__")";
#endif

#define MB_MSG   1
#define MB_JAM   2

struct Area
{
   struct Area *Next;
   uchar Tagname[80];
   uchar Path[80];
   ulong KeepNum,KeepDays;
   int Type;
};

struct Msg
{
   struct Msg *Next;
   ulong Num,NewNum,Day;
};

struct jbList AreaList;
struct jbList MsgList;

#define ARG_MAINT      0
#define ARG_PACK       1
#define ARG_VERBOSE    2
#define ARG_SETTINGS   3
#define ARG_PATTERN    4

struct argument args[] =
   { { ARGTYPE_BOOL,   "MAINT",        NULL },
     { ARGTYPE_BOOL,   "PACK",         NULL },
     { ARGTYPE_BOOL,   "VERBOSE",      NULL },
     { ARGTYPE_STRING, "SETTINGS",     NULL },
     { ARGTYPE_STRING, "PATTERN",      NULL },
     { ARGTYPE_END,     NULL,          0    } };

bool diskfull;
bool ctrlc;

void breakfunc(int x)
{
   ctrlc=TRUE;
}

int Compare(const void *a1,const void *a2)
{
   struct Msg **m1,**m2;

   m1=(struct Msg **)a1;
   m2=(struct Msg **)a2;

   if((*m1)->Num > (*m2)->Num) return(1);
   if((*m1)->Num < (*m2)->Num) return(-1);
   return(0);
}

bool Sort(struct jbList *list)
{
   struct Msg *msg,**buf,**work;
   ulong nc;

   nc=0;

   for(msg=(struct Msg *)list->First;msg;msg=msg->Next)
      nc++;

   if(nc == 0)
      return(TRUE);

   if(!(buf=(struct Msg **)osAlloc(nc*sizeof(struct StatsNode *))))
      return(FALSE);

   work=buf;

   for(msg=(struct Msg *)list->First;msg;msg=msg->Next)
      *work++=msg;

   qsort(buf,nc,4,Compare);

   jbNewList(list);

   for(work=buf;nc--;)
      jbAddNode(list,(struct jbNode *)*work++);

   osFree(buf);

   return(TRUE);
}

void MakeFidoDate(time_t tim,uchar *dest)
{
   struct tm *tp;
   time_t t;
   uchar *monthnames[]={"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec","???"};

   t=tim;
   tp=localtime(&t);

   sprintf(dest,"%02d %s %02d  %02d:%02d:%02d",
      tp->tm_mday,
      monthnames[tp->tm_mon],
      tp->tm_year % 100,
      tp->tm_hour,
      tp->tm_min,
      tp->tm_sec);
}

uchar *scanfuncarea;
bool nomem;

void scanfunc(uchar *str)
{
   uchar buf[200];
   struct osFileEntry *fe;
   ulong num,day;
   struct Msg *msg;

   if(strlen(str) < 5)
      return;

   if(stricmp(&str[strlen(str)-4],".msg")!=0)
      return;
                                                                             
   if(atol(str) < 2)
      return;

   MakeFullPath(scanfuncarea,str,buf,200);

   if(!(fe=osGetFileEntry(buf)))
      return;

   num=atol(str);   
   day=fe->Date / (24*60*60);

   osFree(fe);

   if(!(msg=(struct Msg *)osAlloc(sizeof(struct Msg))))
   {
      nomem=TRUE;
      return;
   }

   jbAddNode(&MsgList,(struct jbNode *)msg);

   msg->Num=num;
   msg->Day=day;
}

bool ProcessAreaMSG(struct Area *area)
{
   ulong today,num,del,highwater,oldhighwater;
   struct Msg *msg;
   uchar buf[200],newbuf[200],buf2[100];
   struct StoredMsg StoredMsg;
   osFile fh;

   highwater=0;
   oldhighwater=0;

   jbNewList(&MsgList);

   printf("Processing %s...\n",area->Tagname);

   scanfuncarea=area->Path;

   if(!(osScanDir(area->Path,scanfunc)))
   {
      printf(" Error: Couldn't scan directory %s\n",area->Path);
      jbFreeList(&MsgList);
      return(TRUE);
   }

   if(nomem)
   {
      printf("Out of memory\n");
      jbFreeList(&MsgList);
      return(FALSE);
   }

   if(!Sort(&MsgList))
   {
      printf("Out of memory\n");
      jbFreeList(&MsgList);
      return(FALSE);
   }

   if(!MsgList.First)
   {
      printf(" Area is empty\n");
      return(TRUE);
   }

   if(ctrlc)
   {
      jbFreeList(&MsgList);
      return(TRUE);
   }

   MakeFullPath(area->Path,"1.msg",buf,200);

   if((fh=osOpen(buf,MODE_OLDFILE)))
   {
      if(osRead(fh,&StoredMsg,sizeof(struct StoredMsg))==sizeof(struct StoredMsg))
      {
         highwater=StoredMsg.ReplyTo;
         oldhighwater=StoredMsg.ReplyTo;
      }
      osClose(fh);
   }

   if(args[ARG_MAINT].data && area->KeepNum!=0)
   {
      num=0;

      for(msg=(struct Msg *)MsgList.First;msg;msg=msg->Next)
         num++;

      msg=(struct Msg *)MsgList.First;
      del=0;

      while(num>area->KeepNum && !ctrlc)
      {
         while(msg->Num==0)
            msg=msg->Next;

         sprintf(buf2,"%lu.msg",msg->Num);
         MakeFullPath(area->Path,buf2,buf,200);

         if(msg->Num == highwater)
            highwater=0;

         if(args[ARG_VERBOSE].data)
            printf(" Deleting message #%lu by number\n",msg->Num);

         remove(buf);

         msg->Num=0;
         num--;
         del++;
      }

      if(ctrlc)
      {
         jbFreeList(&MsgList);
         return(TRUE);
      }

      printf(" %lu messages deleted by number, %lu messages left\n",del,num);
   }

   if(args[ARG_MAINT].data && area->KeepDays!=0)
   {
      del=0;
      num=0;
		
		today=time(NULL) / (24*60*60);

      for(msg=(struct Msg *)MsgList.First;msg && !ctrlc;msg=msg->Next)
      {
         if(today - msg->Day > area->KeepDays && msg->Num!=0)
         {
            sprintf(buf2,"%lu.msg",msg->Num);
            MakeFullPath(area->Path,buf2,buf,200);

            if(msg->Num == highwater)
               highwater=0;

            if(args[ARG_VERBOSE].data)
               printf(" Deleting message #%lu by date\n",msg->Num);

            remove(buf);
            
      	    msg->Num=0;
            del++;
         }
         else
         {
            num++;
         }
      }

      if(ctrlc)
      {
         jbFreeList(&MsgList);
         return(TRUE);
      }

      printf(" %lu messages deleted by date, %lu messages left\n",del,num);
   }

   if(args[ARG_PACK].data)
   {
      num=2;

      msg=(struct Msg *)MsgList.First;

      while(msg && !ctrlc)
      {
         while(msg && msg->Num==0)
            msg=msg->Next;

         if(msg)
         {
            msg->NewNum=num++;
            msg=msg->Next;
         }
      }
 
      for(msg=(struct Msg *)MsgList.First;msg && !ctrlc;msg=msg->Next)
         if(msg->Num!=0 && msg->Num!=msg->NewNum)
         {
            sprintf(buf2,"%lu.msg",msg->Num);
				MakeFullPath(area->Path,buf2,buf,200);

            sprintf(buf2,"%lu.msg",msg->NewNum);
				MakeFullPath(area->Path,buf2,newbuf,200);

            if(highwater == msg->Num)
               highwater=msg->NewNum;

            if(args[ARG_VERBOSE].data)
               printf(" Renaming message %lu to %lu\n",msg->Num,msg->NewNum);

            rename(buf,newbuf);
         }

      if(ctrlc)
      {
         jbFreeList(&MsgList);
         return(TRUE);
      }

      printf(" Area renumbered\n");
   }

   jbFreeList(&MsgList);

   if(highwater!=oldhighwater)
   {
      strcpy(StoredMsg.From,"CrashMail");
      strcpy(StoredMsg.To,"All");
      strcpy(StoredMsg.Subject,"HighWater mark");
      MakeFidoDate(time(NULL),StoredMsg.DateTime);

      StoredMsg.TimesRead=0;
      StoredMsg.DestNode=0;
      StoredMsg.OrigNode=0;
      StoredMsg.Cost=0;
      StoredMsg.OrigNet=0;
      StoredMsg.DestNet=0;
      StoredMsg.DestZone=0;
      StoredMsg.OrigZone=0;
      StoredMsg.OrigPoint=0;
      StoredMsg.DestPoint=0;
      StoredMsg.ReplyTo=highwater;
      StoredMsg.Attr=FLAG_SENT | FLAG_PVT;
      StoredMsg.NextReply=0;

      MakeFullPath(area->Path,"1.msg",buf,200);

      if((fh=osOpen(buf,MODE_NEWFILE)))
      {
         osWrite(fh,&StoredMsg,sizeof(struct StoredMsg));
         osWrite(fh,"",1);
         osClose(fh);
      }
   }

   return(TRUE);
}


bool ProcessAreaJAM(struct Area *area)
{
   ulong today,active,basenum,total,del,num,day;
   s_JamBase *Base_PS,*NewBase_PS;
   s_JamBaseHeader BaseHeader_S;
   s_JamMsgHeader	Header_S;
   s_JamSubPacket*	SubPacket_PS;
	int res;
	uchar buf[200],oldname[200],tmpname[200];
	bool firstwritten;
	uchar *msgtext;
			
   printf("Processing %s...\n",area->Tagname);

   if(JAM_OpenMB(area->Path,&Base_PS))
   {
      printf(" Failed to open messagebase \"%s\"\n",area->Path);
		return(TRUE);
	}
	
	if(JAM_LockMB(Base_PS,10))
	{
		printf(" Timeout when trying to lock messagebase \"%s\"\n",area->Path);
		JAM_CloseMB(Base_PS);
		return(TRUE);
	}

   if(JAM_ReadMBHeader(Base_PS,&BaseHeader_S))
   {
      printf(" Failed to read header of messagebase \"%s\"\n",area->Path);
		JAM_UnlockMB(Base_PS);
		JAM_CloseMB(Base_PS);
      return(TRUE);
   }

   if(JAM_GetMBSize(Base_PS,&total))
   {
      printf(" Failed to get size of messagebase \"%s\"\n",area->Path);
		JAM_UnlockMB(Base_PS);
      JAM_CloseMB(Base_PS);
      return(TRUE);
   }

   basenum=BaseHeader_S.BaseMsgNum;
   active=BaseHeader_S.ActiveMsgs;
	
   if(total == 0)
   {
      printf(" Area is empty\n");
		JAM_UnlockMB(Base_PS);
		JAM_CloseMB(Base_PS);
      return(TRUE);
   }

   if(args[ARG_MAINT].data && area->KeepNum!=0)
   {		
		num=0;
		del=0;
		
		while(num < total && active > area->KeepNum && !ctrlc)
		{
		   res=JAM_ReadMsgHeader(Base_PS,num,&Header_S,NULL);

         if(res == 0)
			{
				/* Read success */
				
			   if(!(Header_S.Attribute & MSG_DELETED))
				{
					/* Not already deleted */

		         if(args[ARG_VERBOSE].data)
      		      printf(" Deleting message #%lu by number\n",basenum+num);
	
					Header_S.Attribute |= MSG_DELETED;
					JAM_ChangeMsgHeader(Base_PS,num,&Header_S);

					BaseHeader_S.ActiveMsgs--;
					JAM_WriteMBHeader(Base_PS,&BaseHeader_S);
					
					active--;
					del++;
				}
			}
			
			num++;
		}

      if(ctrlc)
      {
			JAM_UnlockMB(Base_PS);
			JAM_CloseMB(Base_PS);
         return(TRUE);
      }

      printf(" %lu messages deleted by number, %lu messages left\n",del,active);
   }

   if(args[ARG_MAINT].data && area->KeepDays!=0)
   {
      del=0;
      num=0;
		
		today=time(NULL) / (24*60*60);

		while(num < total && !ctrlc)
		{
		   res=JAM_ReadMsgHeader(Base_PS,num,&Header_S,NULL);

         if(res == 0)
			{
				/* Read success */

				day=Header_S.DateReceived / (24*60*60);
				
				if(day == 0)
					day=Header_S.DateProcessed / (24*60*60);
				
				if(day == 0)
					day=Header_S.DateWritten / (24*60*60);

			   if(today-day > area->KeepDays && !(Header_S.Attribute & MSG_DELETED))
				{
					/* Not already deleted and too old*/

		         if(args[ARG_VERBOSE].data)
      		      printf(" Deleting message #%lu by date\n",basenum+num);
	
					Header_S.Attribute |= MSG_DELETED;
					JAM_ChangeMsgHeader(Base_PS,num,&Header_S);

					BaseHeader_S.ActiveMsgs--;
					JAM_WriteMBHeader(Base_PS,&BaseHeader_S);
					
					del++;
					active--;
				}
			}
			
			num++;
		}

      if(ctrlc)
      {
			JAM_UnlockMB(Base_PS);
			JAM_CloseMB(Base_PS);
         return(TRUE);
      }

      printf(" %lu messages deleted by date, %lu messages left\n",del,active);
   }

   if(args[ARG_PACK].data)
   {
		if(active == total)
		{
			printf(" Messagebase does not need to be packed\n");
			JAM_UnlockMB(Base_PS);
			JAM_CloseMB(Base_PS);
			return(TRUE);
		}

		strcpy(buf,area->Path);
		strcat(buf,".cmtemp");

	   if(JAM_CreateMB(buf,1,&NewBase_PS))
   	{
      	printf(" Failed to create new messagebase \"%s\"\n",buf);
			JAM_UnlockMB(Base_PS);
			JAM_CloseMB(Base_PS);
			return(TRUE);
		}

		if(JAM_LockMB(NewBase_PS,10))
		{
			printf(" Timeout when trying to lock messagebase \"%s\"\n",buf);
			JAM_UnlockMB(Base_PS);
			JAM_CloseMB(Base_PS);
			JAM_CloseMB(NewBase_PS);
			JAM_RemoveMB(NewBase_PS,buf);
			return(TRUE);
		}

		/* Copy messages */

      del=0;
      num=0;
		firstwritten=FALSE;
				
		while(num < total && !ctrlc)
		{
		   res=JAM_ReadMsgHeader(Base_PS,num,&Header_S,NULL);

         if(res)
			{
				if(res == JAM_NO_MESSAGE)
				{
					if(firstwritten) JAM_AddEmptyMessage(NewBase_PS);
					else del++;
				}
				else
				{
		      	printf(" Failed to read message %ld, cannot pack messagebase\n",num+basenum);
					JAM_UnlockMB(Base_PS);
					JAM_CloseMB(Base_PS);
					JAM_UnlockMB(NewBase_PS);
					JAM_CloseMB(NewBase_PS);
					JAM_RemoveMB(NewBase_PS,buf);
					return(TRUE);
				}
			}
			else
			{
				if(Header_S.Attribute & MSG_DELETED)
				{
					if(firstwritten) JAM_AddEmptyMessage(NewBase_PS);
					else del++;
				}
				else
				{
               if(!firstwritten)
               {
                  /* Set basenum */

                  BaseHeader_S.BaseMsgNum+=del;
                  res=JAM_WriteMBHeader(NewBase_PS,&BaseHeader_S);

                  if(res)
                  {
                     printf(" Failed to write messagebase header, cannot pack messagebase\n");
                     JAM_UnlockMB(Base_PS);
                     JAM_CloseMB(Base_PS);
                     JAM_UnlockMB(NewBase_PS);
                     JAM_CloseMB(NewBase_PS);
                     JAM_RemoveMB(NewBase_PS,buf);
                     return(TRUE);
                  }

                  firstwritten=TRUE;
               }

					/* Read header */

					res=JAM_ReadMsgHeader(Base_PS,num,&Header_S,&SubPacket_PS);

				   if(res)
				   {
			      	printf(" Failed to read message %ld, cannot pack messagebase\n",num+basenum);
						JAM_UnlockMB(Base_PS);
						JAM_CloseMB(Base_PS);
						JAM_UnlockMB(NewBase_PS);
						JAM_CloseMB(NewBase_PS);
						JAM_RemoveMB(NewBase_PS,buf);
						return(TRUE);
					}

				   /* Read message text */

				   msgtext=NULL;

				   if(Header_S.TxtLen)
				   {
				      if(!(msgtext=osAlloc(Header_S.TxtLen)))
				      {
							printf("Out of memory\n");
				         JAM_DelSubPacket(SubPacket_PS);
							JAM_UnlockMB(Base_PS);
							JAM_CloseMB(Base_PS);
							JAM_UnlockMB(NewBase_PS);
							JAM_CloseMB(NewBase_PS);
							JAM_RemoveMB(NewBase_PS,buf);
				         return(FALSE);
						}			

				      res=JAM_ReadMsgText(Base_PS,Header_S.TxtOffset,Header_S.TxtLen,msgtext);

				      if(res)
				      {
				      	printf(" Failed to read message %ld, cannot pack messagebase\n",num+basenum);
				         JAM_DelSubPacket(SubPacket_PS);
							JAM_UnlockMB(Base_PS);
							JAM_CloseMB(Base_PS);
							JAM_UnlockMB(NewBase_PS);
							JAM_CloseMB(NewBase_PS);
							JAM_RemoveMB(NewBase_PS,buf);
					      return(TRUE);
						}
			      }

					/* Write new message */

				   res=JAM_AddMessage(NewBase_PS,&Header_S,SubPacket_PS,msgtext,Header_S.TxtLen);

					if(msgtext) osFree(msgtext);
				   JAM_DelSubPacket(SubPacket_PS);

					if(res)
					{
				      printf(" Failed to copy message %ld (disk full?), cannot pack messagebase\n",num+basenum);
						JAM_UnlockMB(Base_PS);
						JAM_CloseMB(Base_PS);
						JAM_UnlockMB(NewBase_PS);
						JAM_CloseMB(NewBase_PS);
						JAM_RemoveMB(NewBase_PS,buf);
					   return(TRUE);
					}
				}
			}
			
			num++;
		}

      if(ctrlc)
      {
			JAM_UnlockMB(Base_PS);
			JAM_CloseMB(Base_PS);
			JAM_UnlockMB(NewBase_PS);
			JAM_CloseMB(NewBase_PS);
			JAM_RemoveMB(NewBase_PS,buf);
         return(TRUE);
      }

		JAM_UnlockMB(Base_PS);
		JAM_CloseMB(Base_PS);

		JAM_UnlockMB(NewBase_PS);
		JAM_CloseMB(NewBase_PS);

		/* This could not be done with JAMLIB... */

	   sprintf(oldname,"%s%s",area->Path,EXT_HDRFILE);
  	   sprintf(tmpname,"%s.cmtemp%s",area->Path,EXT_HDRFILE);
		remove(oldname);
		rename(tmpname,oldname);

	   sprintf(oldname,"%s%s",area->Path,EXT_TXTFILE);
  	   sprintf(tmpname,"%s.cmtemp%s",area->Path,EXT_TXTFILE);
		remove(oldname);
		rename(tmpname,oldname);

	   sprintf(oldname,"%s%s",area->Path,EXT_IDXFILE);
  	   sprintf(tmpname,"%s.cmtemp%s",area->Path,EXT_IDXFILE);
		remove(oldname);
		rename(tmpname,oldname);

	   sprintf(oldname,"%s%s",area->Path,EXT_LRDFILE);
  	   sprintf(tmpname,"%s.cmtemp%s",area->Path,EXT_LRDFILE);
		/* Keep lastread file */
		remove(tmpname);

      printf(" %ld deleted messages removed from messagebase\n",del);
   }
	else
	{
		JAM_UnlockMB(Base_PS);
		JAM_CloseMB(Base_PS);
	}
	
   return(TRUE);
}

uchar cfgbuf[4000];

bool ReadConfig(uchar *file)
{
   osFile fh;
   uchar cfgword[20];
   uchar tag[80];
   uchar path[80];
   struct Area *tmparea,*LastArea;
   ulong jbcpos;
   int type;

   if(!(fh=osOpen(file,MODE_OLDFILE)))
      return(FALSE);

	LastArea=NULL;

   while(osFGets(fh,cfgbuf,4000))
   {
      jbcpos=0;
      jbstrcpy(cfgword,cfgbuf,20,&jbcpos);

      if(stricmp(cfgword,"KEEPDAYS")==0 && LastArea)
      {
         if(jbstrcpy(tag,cfgbuf,80,&jbcpos))
            LastArea->KeepDays=atol(tag);
      }

      if(stricmp(cfgword,"KEEPNUM")==0 && LastArea)
      {
         if(jbstrcpy(tag,cfgbuf,80,&jbcpos))
            LastArea->KeepNum=atol(tag);
      }

      if(stricmp(cfgword,"AREA")==0 || stricmp(cfgword,"NETMAIL")==0)
      {
         jbstrcpy(tag,cfgbuf,80,&jbcpos);
         jbstrcpy(path,cfgbuf,80,&jbcpos);
         jbstrcpy(path,cfgbuf,80,&jbcpos);

         type=0;

         if(stricmp(path,"MSG")==0)
            type=MB_MSG;

         if(stricmp(path,"JAM")==0)
            type=MB_JAM;

         if(type)
         {
            if(stricmp(tag,"DEFAULT")!=0 && strnicmp(tag,"DEFAULT_",8)!=0)
            {
               if(jbstrcpy(path,cfgbuf,80,&jbcpos))
               {
                  if(!(tmparea=(struct Area *)osAllocCleared(sizeof(struct Area))))
                  {
            		   printf("Out of memory\n");
                     osClose(fh);
                     return(FALSE);
                  }

                  jbAddNode(&AreaList,(struct jbNode *)tmparea);
                  LastArea=tmparea;

                  strcpy(tmparea->Tagname,tag);
                  strcpy(tmparea->Path,path);
                  tmparea->Type=type;
               }
            }
         }
      }
   }

   osClose(fh);
   return(TRUE);
}

int main(int argc, char **argv)
{
   struct Area *area;
   uchar *cfg;
   
   signal(SIGINT,breakfunc);

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

   jbNewList(&AreaList);

   if(!args[ARG_MAINT].data && !args[ARG_PACK].data)
   {
      printf("Nothing to do.\n");
      osEnd();
      exit(OS_EXIT_OK);
   }

   if(args[ARG_PATTERN].data)
   {
      if(!(osCheckPattern((uchar *)args[ARG_PATTERN].data)))
      {
         printf("Invalid pattern \"%s\"\n",(uchar *)args[ARG_PATTERN].data);
         osEnd();
         exit(OS_EXIT_ERROR);
      }
   }
  
   cfg=OS_CONFIG_NAME;

   if(args[ARG_SETTINGS].data)
      cfg=(uchar *)args[ARG_SETTINGS].data;

   if(!(ReadConfig(cfg)))
   {
      printf("Couldn't read config \"%s\"",cfg);
      jbFreeList(&AreaList);
      osEnd();
      exit(OS_EXIT_ERROR);
   }

   for(area=(struct Area *)AreaList.First;area && !ctrlc;area=area->Next)
   {
      bool match;

      match=FALSE;

      if(!args[ARG_PATTERN].data)
         match=TRUE;

      else if(osMatchPattern((uchar *)args[ARG_PATTERN].data,area->Tagname))
         match=TRUE;

      if(match)
      {
         if(area->Type == MB_JAM)
         {
            if(!ProcessAreaJAM(area))
               exit(OS_EXIT_ERROR);
         }

         if(area->Type == MB_MSG)
         {
            if(!ProcessAreaMSG(area))
               exit(OS_EXIT_ERROR);
         }
      }
   }

   if(ctrlc)
      printf("*** User Break ***\n");

   jbFreeList(&AreaList);
   osEnd();

   exit(OS_EXIT_OK);
}

