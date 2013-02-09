#include <jamlib/jam.h>

#include "crashmail.h"

#define MIN(a,b)  ((a)<(b)? (a):(b))

struct openbase
{
   uint32_t lastuse;
   s_JamBase* Base_PS;
   struct jam_Area *area;
};

struct jam_Area
{
   struct jam_Area *Next;
   struct Area *area;
   s_JamBase *Base_PS;
   uint32_t BaseNum;
   uint32_t OldNum;
   uint32_t OldHighWater;
   uint32_t HighWater;
   bool newmsg;
};

struct jbList jam_AreaList;
struct openbase *jam_openbases;

uint32_t jam_lastnum;
long jam_utcoffset;

int jam_linkmb(struct Area *area,uint32_t oldnum);

s_JamBase *jam_openbase(struct jam_Area *area)
{
   int c;
   struct openbase *thisbase;

   /* See if that area already is open */

   for(c=0;c<config.cfg_jam_MaxOpen;c++)
      if(jam_openbases[c].area == area && jam_openbases[c].lastuse)
      {
         jam_openbases[c].lastuse=jam_lastnum++;
         return(jam_openbases[c].Base_PS);
      }

   /* We must open it now */

   /* Try to get an empty slot */

   thisbase=NULL;

   for(c=0;c<config.cfg_jam_MaxOpen && !thisbase;c++)
      if(jam_openbases[c].lastuse == 0) thisbase=&jam_openbases[c];

   /* Otherwise, remove a slot that hasn't been used for a long time */

   if(!thisbase)
   {
      thisbase=&jam_openbases[0];

      for(c=0;c<config.cfg_jam_MaxOpen;c++)
         if(jam_openbases[c].lastuse < thisbase->lastuse) thisbase=&jam_openbases[c];

      thisbase->lastuse=0;
      JAM_CloseMB(thisbase->Base_PS);
      free(thisbase->Base_PS);
   }

   /* Open area */

   if(JAM_OpenMB(area->area->Path,&thisbase->Base_PS))
   {
      if(thisbase->Base_PS) free(thisbase->Base_PS);

      LogWrite(2,SYSTEMINFO,"Creating JAM messagebase \"%s\"",area->area->Path);

      if(JAM_CreateMB(area->area->Path,1,&thisbase->Base_PS))
      {
         if(thisbase->Base_PS) free(thisbase->Base_PS);
         LogWrite(1,SYSTEMERR,"Failed to create JAM messagebase \"%s\"",area->area->Path);
         return(NULL);
      }
   }

   /* Set the rest */

   thisbase->lastuse=jam_lastnum++;
   thisbase->area=area;

   return(thisbase->Base_PS);
}

struct jam_Area *jam_getarea(struct Area *area)
{
   struct jam_Area *ja;
   uint32_t num;
   s_JamBaseHeader Header_S;

   /* Check if area already exists */

   for(ja=(struct jam_Area *)jam_AreaList.First;ja;ja=ja->Next)
      if(ja->area == area)
      {
         if(!(ja->Base_PS=jam_openbase(ja)))
            return(NULL);

         return(ja);
      }

   /* This is the first time we use this area */

   if(!(ja=osAllocCleared(sizeof(struct jam_Area))))
   {
      nomem=TRUE;
      return(FALSE);
   }

   jbAddNode(&jam_AreaList,(struct jbNode *)ja);
   ja->area=area;

   if(!(ja->Base_PS=jam_openbase(ja)))
      return(NULL);

   if(JAM_GetMBSize(ja->Base_PS,&num))
   {
      LogWrite(1,TOSSINGERR,"Failed to get size of JAM area \"%s\"",area->Path);
      return(NULL);
   }

   ja->OldNum=num;

   if(JAM_ReadMBHeader(ja->Base_PS,&Header_S))
   {
      LogWrite(1,TOSSINGERR,"Failed to read header of JAM area \"%s\"",area->Path);
      return(NULL);
   }

   ja->BaseNum=Header_S.BaseMsgNum;

   ja->OldHighWater=0;
   ja->HighWater=0;

   return(ja);
}

void jam_gethighwater(struct jam_Area *ja)
{
   char buf[200];
   osFile fh;
   uint32_t num;

   strcpy(buf,ja->area->Path);
   strcat(buf,".cmhw");

   if((fh=osOpen(buf,MODE_OLDFILE)))
   {
      if(osRead(fh,&num,sizeof(uint32_t)))
      {
         ja->HighWater=num;
         ja->OldHighWater=num;
      }

      osClose(fh);
   }
}

void jam_writehighwater(struct jam_Area *ja)
{
   char buf[200];
   osFile fh;
   uint32_t num;

   strcpy(buf,ja->area->Path);
   strcat(buf,".cmhw");
   
   num=ja->HighWater;

   if((fh=osOpen(buf,MODE_NEWFILE)))
   {
      osWrite(fh,&num,sizeof(uint32_t));
      osClose(fh);
   }
}

bool jam_beforefunc(void)
{
   time_t t1,t2;
   struct tm *tp;

   jbNewList(&jam_AreaList);

   if(config.cfg_jam_MaxOpen == 0)
      config.cfg_jam_MaxOpen = 5;

   if(!(jam_openbases=osAllocCleared(config.cfg_jam_MaxOpen * sizeof(struct openbase))))
   {
      nomem=TRUE;
      return(FALSE);
   }

   /* Some timezone tricks */

   t1=time(NULL);
   tp=gmtime(&t1);
   tp->tm_isdst=-1;
   t2=mktime(tp);
   jam_utcoffset=t2-t1;

   jam_lastnum=1;

   return(TRUE);
}

bool jam_afterfunc(bool success)
{
   int c;
   struct jam_Area *ja;

   if(success && (config.cfg_jam_Flags & CFG_JAM_HIGHWATER))
      for(ja=(struct jam_Area *)jam_AreaList.First;ja;ja=ja->Next)
         if(ja->HighWater != ja->OldHighWater)
            jam_writehighwater(ja);

   if(success && (config.cfg_jam_Flags & CFG_JAM_LINK))
      for(ja=(struct jam_Area *)jam_AreaList.First;ja;ja=ja->Next)
         if(ja->newmsg)
            jam_linkmb(ja->area,ja->OldNum);

   for(c=0;c<config.cfg_jam_MaxOpen;c++)
      if(jam_openbases[c].lastuse)
      {
         JAM_CloseMB(jam_openbases[c].Base_PS);
         free(jam_openbases[c].Base_PS);
      }

   osFree(jam_openbases);
   jbFreeList(&jam_AreaList);

	return(TRUE);
}

bool jam_nomem;

void jam_addfield(s_JamSubPacket *SubPacket_PS,uint32_t fieldnum,char *fielddata)
{
   s_JamSubfield	Subfield_S;

   Subfield_S.LoID   = fieldnum;
   Subfield_S.HiID   = 0;
   Subfield_S.DatLen = strlen(fielddata);
   Subfield_S.Buffer = fielddata;

   if(JAM_PutSubfield( SubPacket_PS, &Subfield_S) == JAM_NO_MEMORY)
      jam_nomem=TRUE;
}

struct flag
{
   char *name;
   uint32_t jamflagbit;
   uint32_t fidoflagbit;
};

struct flag jam_flagarray[] =
{ { "PVT", MSG_PRIVATE,     FLAG_PVT         },
  { "HLD", MSG_HOLD,        FLAG_HOLD        },
  { "CRA", MSG_CRASH,       FLAG_CRASH       },
  { "K/S", MSG_KILLSENT,    FLAG_KILLSENT    },
  { "SNT", MSG_SENT,        FLAG_SENT        },
  { "RCV", MSG_READ,        FLAG_RECD        },
  { "A/S", MSG_ARCHIVESENT, 0,               },
  { "DIR", MSG_DIRECT,      0                },
  { "FIL", MSG_FILEATTACH,  FLAG_FILEATTACH  },
  { "FRQ", MSG_FILEREQUEST, FLAG_FILEREQ     },
  { "IMM", MSG_IMMEDIATE,   0                },
  { "KFS", MSG_KILLFILE,    0                },
  { "TFS", MSG_TRUNCFILE,   0                },
  { "LOK", MSG_LOCAL,       FLAG_LOCAL       },
  { "RRQ", MSG_RECEIPTREQ,  FLAG_RREQ        },
  { "CFM", MSG_CONFIRMREQ,  0                },
  { "FPU", MSG_FPU,         0                },
  { "",    MSG_INTRANSIT,   FLAG_INTRANSIT   },
  { "",    MSG_ORPHAN,      FLAG_ORPHAN      }, 
  { NULL,  0,               0                } };

uint32_t jam_findflag(char *name)
{
   int c;

   for(c=0;jam_flagarray[c].name;c++)
      if(stricmp(jam_flagarray[c].name,name)==0) return(jam_flagarray[c].jamflagbit);

   return(0);
}
       
bool jam_importfunc(struct MemMessage *mm,struct Area *area)
{
   struct TextChunk *chunk;
   struct Path *pathnode;
   struct jam_Area *ja;
   s_JamSubPacket*	SubPacket_PS;
   s_JamMsgHeader	Header_S;
   char buf[100],newflags[100],flag[10];
   uint32_t c,f,jbcpos,linebegin,linelen;
   char *msgtext;
   uint32_t msgsize,msgpos;
   int res;

   /* Get an area to write to */

   if(!(ja=jam_getarea(area)))
      return(FALSE);

   /* Start import */

   ja->newmsg=TRUE;

   JAM_ClearMsgHeader(&Header_S);

   if(!(SubPacket_PS = JAM_NewSubPacket()))
   {
      nomem=TRUE;
      return(FALSE);
   }

   /* Allocate memory to store message text in */

   msgpos=0;
   msgsize=0;

   for(chunk=(struct TextChunk *)mm->TextChunks.First;chunk;chunk=chunk->Next)
      msgsize+=chunk->Length;

   if(msgsize != 0)
   {
      if(!(msgtext=osAlloc(msgsize)))
      {
         LogWrite(1,SYSTEMERR,"Out of memory");
         JAM_DelSubPacket(SubPacket_PS);
         return(FALSE);
      }
   }

   /* Do header */

   Header_S.DateProcessed = time(NULL);
   Header_S.DateWritten = FidoToTime(mm->DateTime);

   /* Damned time zones... dates should be in local time in JAM */
   Header_S.DateProcessed -= jam_utcoffset;
   Header_S.DateWritten -= jam_utcoffset;

   Header_S.Cost=mm->Cost;
   Header_S.MsgIdCRC=JAM_Crc32(mm->MSGID,strlen(mm->MSGID));
   Header_S.ReplyCRC=JAM_Crc32(mm->REPLY,strlen(mm->REPLY));

   /* Add header fields */

   if(mm->From[0])
      jam_addfield(SubPacket_PS,JAMSFLD_SENDERNAME,mm->From);

   if(mm->To[0])
      jam_addfield(SubPacket_PS,JAMSFLD_RECVRNAME,mm->To);

   if(mm->Subject[0])
      jam_addfield(SubPacket_PS,JAMSFLD_SUBJECT,mm->Subject);

   if(mm->Area[0] == 0)
   {
      /* Addresses in netmail */

      Print4D(&mm->OrigNode,buf);
      jam_addfield(SubPacket_PS,JAMSFLD_OADDRESS,buf);

      Print4D(&mm->DestNode,buf);
      jam_addfield(SubPacket_PS,JAMSFLD_DADDRESS,buf);
   }
   else
   {
      /* Addresses in echomail */

      Print4D(&mm->Origin4D,buf);
      jam_addfield(SubPacket_PS,JAMSFLD_OADDRESS,buf);
   }

   /* Header attributes */

   for(c=0;jam_flagarray[c].name;c++)
      if(mm->Attr & jam_flagarray[c].fidoflagbit)
         Header_S.Attribute |= jam_flagarray[c].jamflagbit;

   if(mm->Attr & FLAG_FILEATTACH)
   {
      Header_S.Attribute |= MSG_FILEATTACH;

      c=0;

      while(mm->Subject[c]!=0)
      {
         f=0;
         while(mm->Subject[c]!=0 && mm->Subject[c]!=32 && mm->Subject[c]!=',' && f<80)
            buf[f++]=mm->Subject[c++];

         buf[f]=0;

         while(mm->Subject[c]==32 || mm->Subject[c]==',') c++;

         if(buf[0]!=0)
            jam_addfield(SubPacket_PS,JAMSFLD_ENCLFILE,buf);
      }
   }

   if(mm->Attr & FLAG_FILEREQ)
   {
      Header_S.Attribute |= MSG_FILEREQUEST;

      c=0;

      while(mm->Subject[c]!=0)
      {
         f=0;
         while(mm->Subject[c]!=0 && mm->Subject[c]!=32 && mm->Subject[c]!=',' && f<80)
            buf[f++]=mm->Subject[c++];

         buf[f]=0;

         while(mm->Subject[c]==32 || mm->Subject[c]==',') c++;

         if(buf[0]!=0)
            jam_addfield(SubPacket_PS,JAMSFLD_ENCLFREQ,buf);
      }
   }

   /* Echomail/netmail attribute */

   if(mm->Area[0]==0)
      Header_S.Attribute |= MSG_TYPENET;

   else
      Header_S.Attribute |= MSG_TYPEECHO;

   /* Separate kludges from text */

   for(chunk=(struct TextChunk *)mm->TextChunks.First;chunk;chunk=chunk->Next)
      for(c=0;c<chunk->Length;)
      {
			linebegin=msgpos;
			
         while(chunk->Data[c]!=13 && c<chunk->Length)
			{ 
				if(chunk->Data[c]!=10)
					msgtext[msgpos++]=chunk->Data[c];
					
				c++;
			}

         if(chunk->Data[c]==13 && c<chunk->Length)
				msgtext[msgpos++]=chunk->Data[c++];

			linelen=msgpos-linebegin;

         if(linelen!=0)
         {
            if(linelen>=5 && strncmp(&msgtext[linebegin],"\x01""PID:",5)==0)
            {
               mystrncpy(buf,&msgtext[linebegin+5],MIN(100,linelen-5));
               stripleadtrail(buf);
               jam_addfield(SubPacket_PS,JAMSFLD_PID,buf);
					msgpos=linebegin;
            }
            else if(linelen>=7 && strncmp(&msgtext[linebegin],"\x01""MSGID:",7)==0)
            {
               mystrncpy(buf,&msgtext[linebegin+7],MIN(100,linelen-7));
               stripleadtrail(buf);
               jam_addfield(SubPacket_PS,JAMSFLD_MSGID,buf);
					msgpos=linebegin;
            }
            else if(linelen>=7 && strncmp(&msgtext[linebegin],"\x01""REPLY:",7)==0)
            {
               mystrncpy(buf,&msgtext[linebegin+7],MIN(100,linelen-7));
               stripleadtrail(buf);
               jam_addfield(SubPacket_PS,JAMSFLD_REPLYID,buf);
					msgpos=linebegin;
            }
            else if(linelen>=7 && strncmp(&msgtext[linebegin],"\x01""FLAGS:",7)==0)
            {
               mystrncpy(buf,&msgtext[linebegin+7],MIN(100,linelen-7));
               stripleadtrail(buf);

               jbcpos=0;
               newflags[0]=0;

               while(jbstrcpy(flag,buf,10,&jbcpos))
               {
                  uint32_t flagbit;

                  if((flagbit=jam_findflag(flag)))
                  {      
                     Header_S.Attribute |= flagbit;
                  }
                  else
                  {
                     strcat(newflags,flag);
                     strcat(newflags," ");
                  }
               }

               stripleadtrail(newflags);

               if(newflags[0]!=0)
                  jam_addfield(SubPacket_PS,JAMSFLD_FLAGS,newflags);

					msgpos=linebegin;
            }
            else if(linelen>=5 && strncmp(&msgtext[linebegin],"\x01""INTL",5)==0)
            {
               /* Remove this kludge */
					msgpos=linebegin;
            }
            else if(linelen>=5 && strncmp(&msgtext[linebegin],"\x01""TOPT",5)==0)
            {
               /* Remove this kludge */
					msgpos=linebegin;
            }
            else if(linelen>=5 && strncmp(&msgtext[linebegin],"\x01""FMPT",5)==0)
            {
               /* Remove this kludge */
					msgpos=linebegin;
            }
            else if(msgtext[linebegin]==1)
            {
               mystrncpy(buf,&msgtext[linebegin+1],MIN(100,linelen-1));
               stripleadtrail(buf);
               jam_addfield(SubPacket_PS,JAMSFLD_FTSKLUDGE,buf);
					msgpos=linebegin;
            }
         }
      }

   /* Seen-by */

   if(config.cfg_Flags & CFG_IMPORTSEENBY)
   {
      char *buf;
      uint32_t c,d;

      if((buf=mmMakeSeenByBuf(&mm->SeenBy)))
      {
         c=0;

         while(buf[c]!=0)
         {
            d=c;

            while(buf[d]!=0 && buf[d]!=13)
               d++;

            if(buf[d]==13)
            {
               buf[d++]=0;
               jam_addfield(SubPacket_PS,JAMSFLD_SEENBY2D,&buf[c+9]);
            }

            c=d;
         }
      }

      osFree(buf);
   }

   /* Path */

   for(pathnode=(struct Path *)mm->Path.First;pathnode;pathnode=pathnode->Next)
      for(c=0;c<pathnode->Paths;c++)
         jam_addfield(SubPacket_PS,JAMSFLD_PATH2D,pathnode->Path[c]);

   if(jam_nomem)
   {
      LogWrite(1,SYSTEMERR,"Out of memory");
      JAM_DelSubPacket(SubPacket_PS);
      if(msgsize) osFree(msgtext);
      return(FALSE);
   }

   /* Write message */

   if(JAM_LockMB(ja->Base_PS,10))
   {
      LogWrite(1,SYSTEMERR,"Timeout when trying to lock JAM messagebase \"%s\"",area->Path);
      JAM_DelSubPacket(SubPacket_PS);
      if(msgsize) osFree(msgtext);
      return(FALSE);
   }

   if(msgsize == 0)
   {
      msgtext="";
      msgpos=1;
   }

   res=JAM_AddMessage(ja->Base_PS,&Header_S,SubPacket_PS,msgtext,msgpos);

   JAM_UnlockMB(ja->Base_PS);
   JAM_DelSubPacket(SubPacket_PS);
   if(msgsize) osFree(msgtext);

   if(res)
   {
      LogWrite(1,SYSTEMERR,"Failed to write message to JAM messagebase \"%s\"",area->Path);
      return(FALSE);
   }

   return(TRUE);
}

void jam_makekludge(struct MemMessage *mm,char *pre,char *data,uint32_t len)
{
   char *buf;

	if(!(buf=osAlloc(strlen(pre)+len+10))) /* A few bytes extra */
		return;

   strcpy(buf,pre);
   if(len && data) mystrncpy(&buf[strlen(buf)],data,len+1);
   strcat(buf,"\x0d");
   mmAddLine(mm,buf);

	osFree(buf);
}

bool jam_ExportJAMNum(struct Area *area,uint32_t num,bool (*handlefunc)(struct MemMessage *mm),bool isrescanning)
{
   struct MemMessage *mm;
   struct jam_Area *ja;
   char *msgtext;
   char buf[200],domain[20];
   int res,c;
   s_JamSubPacket*      SubPacket_PS;
   s_JamMsgHeader	Header_S;
   s_JamSubfield* Field_PS;
   struct Node4D n4d;
   bool hasaddr;
   char flagsbuf[200],filesubject[200];
   uint16_t oldattr;

   /* Open the area */

   if(!(ja=jam_getarea(area)))
      return(FALSE);

   /* Read message header */

   res=JAM_ReadMsgHeader(ja->Base_PS,num-ja->BaseNum,&Header_S,&SubPacket_PS);

   if(res)
   {
      if(res == JAM_NO_MESSAGE)
      {
         return(TRUE); /* Message no longer exists */
      }
      else
      {
         LogWrite(1,TOSSINGERR,"Failed to read message #%u in JAM messagebase \"%s\"",num,area->Path);
         return(TRUE);
      }
   }

	/* Check if deleted */

   if(Header_S.Attribute & MSG_DELETED)
   {
      /* Message deleted */
      JAM_DelSubPacket(SubPacket_PS);
      return(TRUE);
   }

   /* Check if already sent */

	if(!isrescanning)
	{
		if((Header_S.Attribute & MSG_SENT) || !(Header_S.Attribute & MSG_LOCAL))
		{
			/* Don't touch if the message is sent or not local */
	      JAM_DelSubPacket(SubPacket_PS);
   	   return(TRUE);
		}
   }

   /* Read message text */

   msgtext=NULL;

   if(Header_S.TxtLen)
   {
      if(!(msgtext=osAlloc(Header_S.TxtLen+1))) /* One extra byte for the terminating zero */
      {
         nomem=TRUE;
         JAM_DelSubPacket(SubPacket_PS);
         return(FALSE);
      }

      res=JAM_ReadMsgText(ja->Base_PS,Header_S.TxtOffset,Header_S.TxtLen,msgtext);

      if(res)
      {
         LogWrite(1,TOSSINGERR,"Failed to read message #%u in JAM messagebase \"%s\"",num,area->Path);
         JAM_DelSubPacket(SubPacket_PS);
         return(FALSE);
      }

      msgtext[Header_S.TxtLen]=0;
   }

   /* Allocate message structure */

   if(!(mm=mmAlloc()))
   {
      JAM_DelSubPacket(SubPacket_PS);
      if(msgtext) osFree(msgtext);
      return(FALSE);
   }

   if(area->AreaType == AREATYPE_NETMAIL)
      strcpy(mm->Area,"");

   else
      strcpy(mm->Area,area->Tagname);

   mm->msgnum=num;

   /* Subfields */

   flagsbuf[0]=0;
   filesubject[0]=0;
   hasaddr=FALSE;

   for(Field_PS=JAM_GetSubfield(SubPacket_PS);Field_PS;Field_PS=JAM_GetSubfield(NULL))
   {
      switch(Field_PS->LoID)
      {
         case JAMSFLD_OADDRESS:
            mystrncpy(buf,Field_PS->Buffer,Field_PS->DatLen+1);

            if(Parse5D(buf,&n4d,domain))
            {
               mm->OrigNode.Zone=n4d.Zone;
               mm->OrigNode.Net=n4d.Net;
               mm->OrigNode.Node=n4d.Node;
               mm->OrigNode.Point=n4d.Point;
            }

            break;

         case JAMSFLD_DADDRESS:
            mystrncpy(buf,Field_PS->Buffer,Field_PS->DatLen+1);

            if(hasaddr)
            {
               LogWrite(1,TOSSINGERR,"Warning: Multiple DADDRESS not supported by CrashMail");               }
            else
            {
               hasaddr=TRUE;

               if(Parse5D(buf,&n4d,domain))
               {
                  mm->DestNode.Zone=n4d.Zone;
                  mm->DestNode.Net=n4d.Net;
                  mm->DestNode.Node=n4d.Node;
                  mm->DestNode.Point=n4d.Point;
               }
            }
            break;

         case JAMSFLD_SENDERNAME:
            mystrncpy(buf,Field_PS->Buffer,Field_PS->DatLen+1);
            mystrncpy(mm->From,buf,36);
            break;

         case JAMSFLD_RECVRNAME:
            mystrncpy(buf,Field_PS->Buffer,Field_PS->DatLen+1);
            mystrncpy(mm->To,buf,36);
            break;

         case JAMSFLD_MSGID:
            jam_makekludge(mm,"\x01" "MSGID: ",Field_PS->Buffer,Field_PS->DatLen);
            break;

         case JAMSFLD_REPLYID:
            jam_makekludge(mm,"\x01" "REPLY: ",Field_PS->Buffer,Field_PS->DatLen);
            break;

         case JAMSFLD_SUBJECT:
            mystrncpy(buf,Field_PS->Buffer,Field_PS->DatLen+1);
            mystrncpy(mm->Subject,buf,72);
            break;

         case JAMSFLD_PID:
            jam_makekludge(mm,"\x01" "PID: ",Field_PS->Buffer,Field_PS->DatLen);
            break;

         case JAMSFLD_ENCLFILE:
            if(filesubject[0]) LogWrite(1,TOSSINGERR,"Warning: Multiple ENCLOSEDFILE not supported by CrashMail");
            else mystrncpy(filesubject,Field_PS->Buffer,Field_PS->DatLen+1);
            break;

         case JAMSFLD_ENCLFREQ:
            LogWrite(1,TOSSINGERR,"Warning: ENCLOSEDFREQ not supported by CrashMail");
            break;

         case JAMSFLD_ENCLFWALIAS:
            LogWrite(1,TOSSINGERR,"Warning: ENCLOSEDFILEWALIAS not supported by CrashMail");
            break;

         case JAMSFLD_ENCLFILEWC:
            LogWrite(1,TOSSINGERR,"Warning: ENCLOSEDFILEWCARD with wildcards not supported by CrashMail");
            break;

         case JAMSFLD_ENCLINDFILE:
            LogWrite(1,TOSSINGERR,"Warning: ENCLOSEDINDIRECTFILE not supported by CrashMail");
            break;

         case JAMSFLD_FTSKLUDGE:
            jam_makekludge(mm,"\x01",Field_PS->Buffer,Field_PS->DatLen);
            break;

         case JAMSFLD_SEENBY2D:
            jam_makekludge(mm,"SEEN-BY: ",Field_PS->Buffer,Field_PS->DatLen);
            break;

         case JAMSFLD_PATH2D:
            jam_makekludge(mm,"\01" "PATH: ",Field_PS->Buffer,Field_PS->DatLen);
            break;

         case JAMSFLD_FLAGS:
            strcpy(flagsbuf,"\x01" "FLAGS: ");
            mystrncpy(&flagsbuf[8],Field_PS->Buffer,Field_PS->DatLen+1);
            /* Don't add until attributes from header has been added */
            break;
      }
   }

   if(filesubject[0])
   {
      mm->Attr|=FLAG_FILEATTACH;
      mystrncpy(mm->Subject,filesubject,72);
   }

   /* Message header */

   MakeFidoDate(Header_S.DateWritten+jam_utcoffset,mm->DateTime);
   mm->Cost=Header_S.Cost;

   for(c=0;jam_flagarray[c].name;c++)
      if(Header_S.Attribute & jam_flagarray[c].jamflagbit)
      {
         if(jam_flagarray[c].fidoflagbit)
         {
            mm->Attr |= jam_flagarray[c].fidoflagbit;
         }
         else if(jam_flagarray[c].name[0] && strlen(flagsbuf)<90)
         {
            if(flagsbuf[0]==0) strcpy(flagsbuf,"\x01" "FLAGS: ");
            else               strcat(flagsbuf," ");

            strcat(flagsbuf,jam_flagarray[c].name);
         }
      }

   if(flagsbuf[0])
   {
      strcat(flagsbuf,"\x0d");
      mmAddLine(mm,buf);
   }

   oldattr = mm->Attr;

   mm->Attr = mm->Attr & (FLAG_PVT|FLAG_CRASH|FLAG_FILEATTACH|FLAG_FILEREQ|FLAG_RREQ|FLAG_IRRR|FLAG_AUDIT|FLAG_HOLD);

   /* Add own kludges */

   if(area->AreaType == AREATYPE_NETMAIL)
   {
      if(mm->OrigNode.Zone != mm->DestNode.Zone || (config.cfg_Flags & CFG_FORCEINTL))
      {
         sprintf(buf,"\x01" "INTL %u:%u/%u %u:%u/%u\x0d",
            mm->DestNode.Zone,
            mm->DestNode.Net,
            mm->DestNode.Node,
            mm->OrigNode.Zone,
            mm->OrigNode.Net,
            mm->OrigNode.Node);

         mmAddLine(mm,buf);
      }

      if(mm->OrigNode.Point)
      {
         sprintf(buf,"\x01" "FMPT %u\x0d",mm->OrigNode.Point);
         mmAddLine(mm,buf);
      }

      if(mm->DestNode.Point)
      {
         sprintf(buf,"\x01" "TOPT %u\x0d",mm->DestNode.Point);
         mmAddLine(mm,buf);
      }
   }

   if((config.cfg_Flags & CFG_ADDTID) && !isrescanning)
      AddTID(mm);

   if(isrescanning)
   {
      sprintf(buf,"\x01RESCANNED %u:%u/%u.%u\x0d",area->Aka->Node.Zone,
                                                  area->Aka->Node.Net,
                                                  area->Aka->Node.Node,
                                                  area->Aka->Node.Point);
      mmAddLine(mm,buf);
   }

   /* Message text */

   if(msgtext)
   {
      /* Extract origin address */

      if(mm->Area[0])
      {
         uint32_t textpos,d;
         char originbuf[200];
         struct Node4D n4d;

         textpos=0;

         while(msgtext[textpos])
         {
            d=textpos;

            while(msgtext[d] != 13 && msgtext[d] != 0)
               d++;

            if(msgtext[d] == 13)
               d++;

            if(d-textpos > 11 && strncmp(&msgtext[textpos]," * Origin: ",11)==0)
            {
               mystrncpy(originbuf,&msgtext[textpos],MIN(d-textpos,200));

               if(ExtractAddress(originbuf,&n4d))
         			Copy4D(&mm->Origin4D,&n4d);
            }

            textpos=d;
         }
      }

      if(!(mmAddBuf(&mm->TextChunks,msgtext,Header_S.TxtLen)))
      {
         JAM_DelSubPacket(SubPacket_PS);
         osFree(msgtext);
         mmFree(mm);
         return(FALSE);
      }
   }

   /* Free JAM message */

   if(msgtext) osFree(msgtext);
   JAM_DelSubPacket(SubPacket_PS);

   /* Message reading done */

   if(isrescanning) mm->Flags |= MMFLAG_RESCANNED;
   else             mm->Flags |= MMFLAG_EXPORTED;

   if(!(*handlefunc)(mm))
   {
      mmFree(mm);
      return(FALSE);
   }

   if(!isrescanning)
   {
      scan_total++;

	   /* Update message header */

		if(config.cfg_Flags & CFG_ALLOWKILLSENT)
		{
			if((oldattr & FLAG_KILLSENT) && (area->AreaType == AREATYPE_NETMAIL))
			{
				/* Delete message with KILLSENT flag */

				LogWrite(2,TOSSINGINFO,"Deleting message with KILLSENT flag");
		   	Header_S.Attribute |= MSG_DELETED;
			}
		}

	   Header_S.Attribute |= MSG_SENT;

      Header_S.DateProcessed = time(NULL);
      Header_S.DateProcessed -= jam_utcoffset;

      /* jam_openbases might have been changed in handlefunc */

      if(!(ja=jam_getarea(area)))
      {
         mmFree(mm);
         return(FALSE);
      }

      if(JAM_LockMB(ja->Base_PS,10))
   	{
      	LogWrite(1,SYSTEMERR,"Timeout when trying to lock JAM messagebase \"%s\"",area->Path);
         mmFree(mm);
         return(FALSE);
	   }

      if(JAM_ChangeMsgHeader(ja->Base_PS,num-ja->BaseNum,&Header_S))
         LogWrite(1,TOSSINGERR,"Failed to update header of message #%u in JAM messagebase \"%s\"",num,area->Path);

      JAM_UnlockMB(ja->Base_PS);
   }

   mmFree(mm);
   return(TRUE);
}

bool jam_exportfunc(struct Area *area,bool (*handlefunc)(struct MemMessage *mm))
{
   uint32_t start,end;
   struct jam_Area *ja;

   /* Open the area */

   if(!(ja=jam_getarea(area)))
	{
		if(nomem)
	      return(FALSE);

		return(TRUE); /* Area did not exist and could not be created. Go on anyway. */
	}
	
   if(config.cfg_jam_Flags & CFG_JAM_HIGHWATER)
      jam_gethighwater(ja);

   if(ja->HighWater) start=ja->HighWater+1;
   else              start=ja->BaseNum;

	if(start < ja->BaseNum)
		start=ja->BaseNum;

   end   = ja->BaseNum + ja->OldNum;

   while(start < end && !ctrlc)
   {
      if(!jam_ExportJAMNum(area,start,handlefunc,FALSE))
         return(FALSE);

      start++;
   }

   if(ctrlc)
      return(FALSE);

   ja->HighWater=end-1;

   return(TRUE);
}

bool jam_rescanfunc(struct Area *area,uint32_t max,bool (*handlefunc)(struct MemMessage *mm))
{
   uint32_t start;
   struct jam_Area *ja;

   /* Open the area */

   if(!(ja=jam_getarea(area)))
      return(FALSE);

   start=ja->BaseNum;

   if(max !=0 && ja->OldNum > max)
      start=ja->BaseNum+ja->OldNum-max;

   while(start < ja->BaseNum + ja->OldNum && !ctrlc)
   {
      if(!jam_ExportJAMNum(area,start,handlefunc,TRUE))
         return(FALSE);

      start++;
   }

   if(ctrlc)
      return(FALSE);

   return(TRUE);
}

/************************** Linking ***********************/

struct Msg
{
   unsigned long MsgIdCRC;
   unsigned long ReplyCRC;
   unsigned long ReplyTo;
   unsigned long Reply1st;
   unsigned long ReplyNext;
   unsigned long OldReplyTo;
   unsigned long OldReply1st;
   unsigned long OldReplyNext;
};

int jam_CompareMsgIdReply(s_JamBase *Base_PS,struct Msg *msgs,uint32_t msgidmsg,uint32_t replymsg)
{
   int Status_I;
   s_JamMsgHeader 	MsgIdHeader_S;
   s_JamMsgHeader 	ReplyHeader_S;
   s_JamSubPacket*   MsgIdSubPacket_PS;
   s_JamSubPacket*   ReplySubPacket_PS;
   s_JamSubfield*    MsgIdField_PS = NULL;
   s_JamSubfield*    ReplyField_PS = NULL;

   if(msgs[msgidmsg].MsgIdCRC != msgs[replymsg].ReplyCRC)
      return(FALSE);

   if(config.cfg_jam_Flags & CFG_JAM_QUICKLINK)
      return(TRUE);

   Status_I = JAM_ReadMsgHeader(Base_PS,msgidmsg,&MsgIdHeader_S,&MsgIdSubPacket_PS );

   if(Status_I)
      return(FALSE);

   Status_I = JAM_ReadMsgHeader(Base_PS,replymsg,&ReplyHeader_S,&ReplySubPacket_PS );

   if(Status_I)
   {
      JAM_DelSubPacket(MsgIdSubPacket_PS);
      return(FALSE);
   }

   for ( MsgIdField_PS = JAM_GetSubfield( MsgIdSubPacket_PS ); MsgIdField_PS; MsgIdField_PS = JAM_GetSubfield( NULL ) )
      if(MsgIdField_PS->LoID == JAMSFLD_MSGID) break;

   for ( ReplyField_PS = JAM_GetSubfield( ReplySubPacket_PS ); ReplyField_PS; ReplyField_PS = JAM_GetSubfield( NULL ) )
      if(ReplyField_PS->LoID == JAMSFLD_REPLYID) break;

   if(!ReplyField_PS || !MsgIdField_PS)
   {
      JAM_DelSubPacket(MsgIdSubPacket_PS);
      JAM_DelSubPacket(ReplySubPacket_PS);
      return(FALSE);
   }

   if(ReplyField_PS->DatLen != MsgIdField_PS->DatLen)
   {
      JAM_DelSubPacket(MsgIdSubPacket_PS);
      JAM_DelSubPacket(ReplySubPacket_PS);
      return(FALSE);
   }

   if(strncmp(ReplyField_PS->Buffer,MsgIdField_PS->Buffer,ReplyField_PS->DatLen) != 0)
   {
      JAM_DelSubPacket(MsgIdSubPacket_PS);
      JAM_DelSubPacket(ReplySubPacket_PS);
      return(FALSE);
   }

   JAM_DelSubPacket(MsgIdSubPacket_PS);
   JAM_DelSubPacket(ReplySubPacket_PS);

   return(TRUE);
}

/*  dest is a reply to num */
void jam_setreply(struct Msg *msgs,uint32_t nummsgs,uint32_t base,uint32_t num,uint32_t dest)
{
   int n,times;

   if(msgs[dest].ReplyTo)
      return; /* Already linked */

   msgs[dest].ReplyTo=num+base;

   if(msgs[num].Reply1st == 0)
   {
      msgs[num].Reply1st=dest+base;
   }
   else
   {
      n=msgs[num].Reply1st-base;
      if(n == dest) return;

      if (n < 0 || n >= nummsgs)
      {
         /* Oops! Base seems to be b0rken */
         printf("Warning: message #%d is linked to something outside the base\n", num + base);
         return;
      }

      times=0;

      while(msgs[n].ReplyNext)
      {
			times++;

			if(times > 1000) /* Something appears to have gone wrong */
			{
		      printf("Warning: >1000 replies to message %d or circular reply links\n",num+base);
				return;
			}

         n=msgs[n].ReplyNext-base;
         if(n == dest) return;

         if (n < 0 || n >= nummsgs)
         {
            /* Oops! Base seems to be b0rken */
            printf("Warning: message #%d is linked to something outside the base\n", num + base);
            return;
         }
      }

      msgs[n].ReplyNext=dest+base;
   }
}

int jam_linkmb(struct Area *area,uint32_t oldnum)
{
   struct jam_Area *ja;
   uint32_t nummsgs,res,c,d;
   struct Msg *msgs;

   printf("Linking JAM area %s                       \n",area->Tagname);
   fflush(stdout);

   if(!(ja=jam_getarea(area)))
      return(FALSE);

   if(JAM_GetMBSize(ja->Base_PS,&nummsgs))
   {
      LogWrite(1,TOSSINGERR,"Failed to get size of JAM area \"%s\"",area->Path);
      return(FALSE);
   }

   if(nummsgs == 0)
      return(TRUE); /* Nothing to do */

   /* Read msgid/reply */

   if(!(msgs=osAlloc(nummsgs*sizeof(struct Msg))))
   {
      LogWrite(1,SYSTEMERR,"Out of memory, cannot link JAM area %s",area->Tagname);
      return(FALSE);
   }

   for(c=0;c<nummsgs;c++)
   {
      s_JamMsgHeader         Header_S;

      res = JAM_ReadMsgHeader( ja->Base_PS, c, &Header_S, NULL);

      msgs[c].MsgIdCRC=-1;
      msgs[c].ReplyCRC=-1;
      msgs[c].ReplyTo=0;
      msgs[c].Reply1st=0;
      msgs[c].ReplyNext=0;
      msgs[c].OldReplyTo=0;
      msgs[c].OldReply1st=0;
      msgs[c].OldReplyNext=0;

      if(!res)
      {
         msgs[c].MsgIdCRC=Header_S.MsgIdCRC;
         msgs[c].ReplyCRC=Header_S.ReplyCRC;
         msgs[c].ReplyTo=Header_S.ReplyTo;
         msgs[c].Reply1st=Header_S.Reply1st;
         msgs[c].ReplyNext=Header_S.ReplyNext;
         msgs[c].OldReplyTo=Header_S.ReplyTo;
         msgs[c].OldReply1st=Header_S.Reply1st;
         msgs[c].OldReplyNext=Header_S.ReplyNext;
      }
   }

   for(c=oldnum;c<nummsgs;c++)
   {
      if(msgs[c].ReplyCRC != -1)
      {
         /* See if this is a reply to a message */

         for(d=0;d<nummsgs;d++)
            if(jam_CompareMsgIdReply(ja->Base_PS,msgs,d,c))
               jam_setreply(msgs,nummsgs,ja->BaseNum,d,c);
      }

      if(msgs[c].MsgIdCRC != -1)
      {
	      /* See if there are any replies to this message */

         for(d=0;d<nummsgs;d++)
            if(jam_CompareMsgIdReply(ja->Base_PS,msgs,c,d))
               jam_setreply(msgs,nummsgs,ja->BaseNum,c,d);
      }
   }

   /* Update links */

   for(c=0;c<nummsgs;c++)
      if(msgs[c].ReplyTo != msgs[c].OldReplyTo || msgs[c].Reply1st != msgs[c].OldReply1st || msgs[c].ReplyNext != msgs[c].OldReplyNext)
      {
         s_JamMsgHeader         Header_S;
  
         if(JAM_LockMB(ja->Base_PS,10))
         {
            LogWrite(1,SYSTEMERR,"Timeout when trying to lock JAM messagebase \"%s\"",area->Path);
            osFree(msgs);
				return(FALSE);
         }

         res = JAM_ReadMsgHeader( ja->Base_PS, c, &Header_S, NULL);

         if(!res)
         {
            Header_S.ReplyTo=msgs[c].ReplyTo;
            Header_S.Reply1st=msgs[c].Reply1st;
            Header_S.ReplyNext=msgs[c].ReplyNext;

            JAM_ChangeMsgHeader(ja->Base_PS,c,&Header_S);
            JAM_UnlockMB(ja->Base_PS);
         }
      }

   osFree(msgs);

	return(TRUE);
}
