#include "crashmail.h"

/*************************************************************************/
/*                                                                       */
/*                                Read Pkt                               */
/*                                                                       */
/*************************************************************************/

#define PKT_MINREADLEN 200

bool messageend;
bool shortread,longread;

uint16_t getuword(uint8_t *buf,uint32_t offset)
{
   return (uint16_t)(buf[offset]+256*buf[offset+1]);
}

void putuword(uint8_t *buf,uint32_t offset,uint16_t num)
{
   buf[offset]=num%256;
   buf[offset+1]=num/256;
}

uint32_t ReadNull(char *buf, uint32_t maxlen, osFile fh)
{
   /* Reads from fh until buffer full or NULL */

   short ch,c=0;

   if(shortread) return(0);

   ch=osGetChar(fh);

   while(ch!=-1 && ch!=0 && c!=maxlen-1)
   {
      buf[c++]=ch;
      ch=osGetChar(fh);
   }
   buf[c]=0;

   if(ch==-1)
      shortread=TRUE;

   if(ch!=0 && c==maxlen-1)
      longread=TRUE;

   return(c);
}

uint32_t ReadCR(char *buf, uint32_t maxlen, osFile fh)
{
   /* Reads from fh until buffer full or CR */

   short ch,c=0;

   ch=osGetChar(fh);

   while(ch!=-1 && ch!=0 && ch!=10 && ch !=13 && c!=maxlen-2)
   {
      buf[c++]=ch;
      if(c!=maxlen-2) ch=osGetChar(fh);
   }

   if(ch==13 || ch==10)
      buf[c++]=ch;

   buf[c]=0;

   if(ch==0)  messageend=TRUE;
   if(ch==-1) shortread=TRUE;

   return(c);
}

bool ReadPkt(char *pkt,struct osFileEntry *fe,bool bundled,bool (*handlefunc)(struct MemMessage *mm))
{
   osFile fh;
   struct Aka *tmpaka;
   struct ConfigNode *tmpcnode;
   uint32_t msgnum,msgoffset;
   char buf[200];
   uint8_t PktHeader[SIZE_PKTHEADER];
   uint8_t PktMsgHeader[SIZE_PKTMSGHEADER];
   struct Node4D PktOrig,PktDest;
   char *monthnames[]={"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec","???"};
   struct MemMessage *mm;
   struct TextChunk *chunk;
   bool pkt_pw,pkt_4d,pkt_5d;
   int res;

   if(config.cfg_BeforeToss[0])
   {
      ExpandPacker(config.cfg_BeforeToss,buf,200,"",pkt);
      res=osExecute(buf);

      if(res != 0)
      {
            LogWrite(1,SYSTEMERR,"BEFORETOSS command failed for %s: %u",pkt,res);
         return(FALSE);
      }
   }

   shortread=FALSE;
   longread=FALSE;

   pkt_pw=FALSE;
   pkt_4d=FALSE;
   pkt_5d=FALSE;

   PktOrig.Zone=0;
   PktOrig.Net=0;
   PktOrig.Node=0;
   PktOrig.Point=0;

   PktDest.Zone=0;
   PktDest.Net=0;
   PktDest.Node=0;
   PktDest.Point=0;

   if(!(mm=mmAlloc()))
      return(FALSE);

   if(!(fh=osOpen(pkt,MODE_OLDFILE)))
   {
		uint32_t err=osError();
      LogWrite(1,SYSTEMERR,"Unable to open %s",pkt);
		LogWrite(1,SYSTEMERR,"Error: %s",osErrorMsg(err));
      mmFree(mm);
      return(TRUE);
   }

   if(osRead(fh,PktHeader,SIZE_PKTHEADER)!=SIZE_PKTHEADER)
   {
      LogWrite(1,TOSSINGERR,"Packet header in %s is too short",pkt);
      osClose(fh);
      mmFree(mm);
      BadFile(pkt,"Packet header is too short");
      return(TRUE);
   }

   if(getuword(PktHeader,PKTHEADER_PKTTYPE)!=0x0002)
   {
      LogWrite(1,TOSSINGERR,"%s is not a Type-2 packet",pkt);
      osClose(fh);
      mmFree(mm);
      BadFile(pkt,"Not a Type-2 packet");
      return(TRUE);
   }

   if(getuword(PktHeader,PKTHEADER_BAUD) == 2)
   {
      /* PktOrig och PktDest */

      PktOrig.Zone  = getuword(PktHeader,PKTHEADER_ORIGZONE);
      PktOrig.Net   = getuword(PktHeader,PKTHEADER_ORIGNET);
      PktOrig.Node  = getuword(PktHeader,PKTHEADER_ORIGNODE);
      PktOrig.Point = getuword(PktHeader,PKTHEADER_ORIGPOINT);

      PktDest.Zone  = getuword(PktHeader,PKTHEADER_DESTZONE);
      PktDest.Net   = getuword(PktHeader,PKTHEADER_DESTNET);
      PktDest.Node  = getuword(PktHeader,PKTHEADER_DESTNODE);
      PktDest.Point = getuword(PktHeader,PKTHEADER_DESTPOINT);


      pkt_5d=TRUE;
   }
   else
   {
      /* PktOrig och PktDest */

      if(getuword(PktHeader,PKTHEADER_ORIGZONE))
      {
         PktOrig.Zone = getuword(PktHeader,PKTHEADER_ORIGZONE);
         PktDest.Zone = getuword(PktHeader,PKTHEADER_DESTZONE);
      }
      else if(getuword(PktHeader,PKTHEADER_QORIGZONE))
      {
         PktOrig.Zone= getuword(PktHeader,PKTHEADER_QORIGZONE);
         PktDest.Zone= getuword(PktHeader,PKTHEADER_QDESTZONE);
      }
      else
      {
         PktOrig.Zone=0;
         PktDest.Zone=0;
      }

      PktOrig.Net  = getuword(PktHeader,PKTHEADER_ORIGNET);
      PktOrig.Node = getuword(PktHeader,PKTHEADER_ORIGNODE);
      PktDest.Net  = getuword(PktHeader,PKTHEADER_DESTNET);
      PktDest.Node = getuword(PktHeader,PKTHEADER_DESTNODE);

      if(PktHeader[PKTHEADER_CWVALIDCOPY] == PktHeader[PKTHEADER_CAPABILWORD+1] &&
         PktHeader[PKTHEADER_CWVALIDCOPY+1] == PktHeader[PKTHEADER_CAPABILWORD])
      {
         pkt_4d=TRUE;

         if(getuword(PktHeader,PKTHEADER_ORIGPOINT)!=0 && getuword(PktHeader,PKTHEADER_ORIGNET)==0xffff)
            PktOrig.Net = getuword(PktHeader,PKTHEADER_AUXNET);

         PktOrig.Point = getuword(PktHeader,PKTHEADER_ORIGPOINT);
         PktDest.Point = getuword(PktHeader,PKTHEADER_DESTPOINT);
      }
   }

   /* Check packet destination */

   if((config.cfg_Flags & CFG_CHECKPKTDEST) && !no_security)
   {
      for(tmpaka=(struct Aka *)config.AkaList.First;tmpaka;tmpaka=tmpaka->Next)
         if(Compare4D(&tmpaka->Node,&PktDest) == 0) break;

      if(!tmpaka)
      {
         LogWrite(1,TOSSINGERR,"Addressed to %u:%u/%u.%u, not to me",PktDest.Zone,PktDest.Net,PktDest.Node,PktDest.Point);
         osClose(fh);
         mmFree(mm);
         sprintf(buf,"Addressed to %u:%u/%u.%u, not to me",PktDest.Zone,PktDest.Net,PktDest.Node,PktDest.Point);
         BadFile(pkt,buf);
         return(TRUE);
      }
   }

   /* Fixa zone */

   if(PktOrig.Zone == 0)
      for(tmpcnode=(struct ConfigNode *)config.CNodeList.First;tmpcnode;tmpcnode=tmpcnode->Next)
      {
         if(Compare4D(&PktOrig,&tmpcnode->Node)==0)
         {
            PktOrig.Zone=tmpcnode->Node.Zone;
            break;
         }
      }

   if(PktDest.Zone == 0)
      for(tmpaka=(struct Aka *)config.AkaList.First;tmpaka;tmpaka=tmpaka->Next)
      {
         if(Compare4D(&PktDest,&tmpaka->Node)==0)
         {
            PktDest.Zone=tmpaka->Node.Zone;
            break;
         }
      }

   if(PktOrig.Zone == 0) PktOrig.Zone=config.cfg_DefaultZone;
   if(PktDest.Zone == 0) PktDest.Zone=config.cfg_DefaultZone;

   for(tmpcnode=(struct ConfigNode *)config.CNodeList.First;tmpcnode;tmpcnode=tmpcnode->Next)
      if(Compare4D(&PktOrig,&tmpcnode->Node)==0) break;

   if(tmpcnode)
   {
      if(tmpcnode->PacketPW[0]!=0 && PktHeader[PKTHEADER_PASSWORD]!=0)
         pkt_pw=TRUE;
   }

   buf[0]=0;

   if(pkt_pw) strcat(buf,", pw");
   if(pkt_4d) strcat(buf,", 4d");
   if(pkt_5d) strcat(buf,", 5d");

   if(buf[0] != 0)
      buf[0]='/';

   if(pkt_5d)
   {
      char domain[10];

      mystrncpy(domain,(char *)&PktHeader[PKTHEADER45_ORIGDOMAIN],9);

      LogWrite(1,ACTIONINFO,"Tossing %s (%uK) from %d:%d/%d.%d@%s %s",
                                                              fe->Name,
                                                              (fe->Size+512)/1024,
                                                              PktOrig.Zone,
                                                              PktOrig.Net,
                                                              PktOrig.Node,
                                                              PktOrig.Point,
                                                              domain,
                                                              buf);
   }
   else
   {
      int month;

      month=getuword(PktHeader,PKTHEADER_MONTH);

      if(month > 11)
         month=12;

      LogWrite(1,ACTIONINFO,"Tossing %s (%uK) from %d:%d/%d.%d (%02d-%s-%02d %02d:%02d:%02d) %s",
         fe->Name,
         (fe->Size+512)/1024,
         PktOrig.Zone,
         PktOrig.Net,
         PktOrig.Node,
         PktOrig.Point,
         getuword(PktHeader,PKTHEADER_DAY),
         monthnames[month],
         getuword(PktHeader,PKTHEADER_YEAR) % 100,
         getuword(PktHeader,PKTHEADER_HOUR),
         getuword(PktHeader,PKTHEADER_MINUTE),
         getuword(PktHeader,PKTHEADER_SECOND),
         buf);
   }

   if(tmpcnode)
   {
      strncpy(buf,(char *)&PktHeader[PKTHEADER_PASSWORD],8);
      buf[8]=0;

      if(tmpcnode->PacketPW[0]!=0 && stricmp(buf,tmpcnode->PacketPW)!=0 && !no_security)
      {
         LogWrite(1,TOSSINGERR,"Wrong password");
         osClose(fh);
         mmFree(mm);
         BadFile(pkt,"Wrong password");
         return(TRUE);
      }
   }

   msgoffset=osFTell(fh);

   if(osRead(fh,PktMsgHeader,SIZE_PKTMSGHEADER) < 2)
   {
      LogWrite(1,TOSSINGERR,"Message header for msg #1 (offset %d) is too short",msgoffset);
      osClose(fh);
      mmFree(mm);
      sprintf(buf,"Message header for msg #1 (offset %d) is too short",msgoffset);
      BadFile(pkt,buf);
      return(TRUE);
   }

   msgnum=0;

   while(getuword(PktMsgHeader,PKTMSGHEADER_PKTTYPE) == 2 && !ctrlc)
   {
      msgnum++;

      printf("Message %u              \x0d",msgnum);
      fflush(stdout);

      /* Init variables */

      mmClear(mm);

      mm->OrigNode.Net  = getuword(PktMsgHeader,PKTMSGHEADER_ORIGNET);
      mm->OrigNode.Node = getuword(PktMsgHeader,PKTMSGHEADER_ORIGNODE);

      mm->DestNode.Net  = getuword(PktMsgHeader,PKTMSGHEADER_DESTNET);
      mm->DestNode.Node = getuword(PktMsgHeader,PKTMSGHEADER_DESTNODE);

      mm->DestNode.Zone=PktDest.Zone;
      mm->DestNode.Zone=PktOrig.Zone;

      mm->Attr=getuword(PktMsgHeader,PKTMSGHEADER_ATTR);
      mm->Cost=getuword(PktMsgHeader,PKTMSGHEADER_COST);

      Copy4D(&mm->PktOrig,&PktOrig);
      Copy4D(&mm->PktDest,&PktDest);

      if(no_security)
         mm->Flags |= MMFLAG_NOSECURITY;

      /* Get header strings */

      ReadNull(mm->DateTime,20,fh);
      ReadNull(mm->To,36,fh);
      ReadNull(mm->From,36,fh);
      ReadNull(mm->Subject,72,fh);

      /* Corrupt packet? */

      if(shortread)
      {
         LogWrite(1,TOSSINGERR,"Message header for msg #%u (offset %d) is short",msgnum,msgoffset);
         sprintf(buf,"Message header for msg #%u (offset %d) is short",msgnum,msgoffset);
         osClose(fh);
         mmFree(mm);
         BadFile(pkt,buf);
         return(TRUE);
      }

      if(longread)
      {
         LogWrite(1,TOSSINGERR,"Header strings too long in msg #%u (offset %d)",msgnum,msgoffset);
         sprintf(buf,"Header strings too long in msg #%u (offset %d)",msgnum,msgoffset);
         osClose(fh);
         mmFree(mm);
         BadFile(pkt,buf);
         return(TRUE);
      }

      /* Start reading message text */

      messageend=FALSE;

      ReadCR(buf,200,fh);

      /* Echomail or netmail? */

      if(strncmp(buf,"AREA:",5)==0)
      {
         mystrncpy(mm->Area,&buf[5],80);
         stripleadtrail(mm->Area); /* Strip spaces from area name */
      }
      else
      {
         if(!mmAddLine(mm,buf))
         {
            osClose(fh);
            mmFree(mm);
            return(FALSE);
         }
      }

      /* Get rest of text */

      while(!messageend)
      {
         ReadCR(buf,200,fh);

         if(shortread)
         {
            osClose(fh);
            mmFree(mm);
            LogWrite(1,TOSSINGERR,"Got unexpected EOF when reading msg #%u (offset %d)",msgnum,msgoffset);
            sprintf(buf,"Got unexpected EOF when reading msg #%u (offset %d)",msgnum,msgoffset);
            BadFile(pkt,buf);
            return(TRUE);
         }

         if(buf[0])
         {
            if(!mmAddLine(mm,buf))
            {
               osClose(fh);
               mmFree(mm);
               return(FALSE);
            }
         }
      }

      /* Stats */
      
      for(tmpcnode=(struct ConfigNode *)config.CNodeList.First;tmpcnode;tmpcnode=tmpcnode->Next)
         if(Compare4D(&tmpcnode->Node,&mm->PktOrig)==0) break;
         
      if(tmpcnode)
      {
         uint32_t size;
         
         size=0;

         for(chunk=(struct TextChunk *)mm->TextChunks.First;chunk;chunk=chunk->Next)
            size+=chunk->Length;

         if(mm->Area[0])
         {
            tmpcnode->GotEchomails++;
            tmpcnode->GotEchomailBytes+=size;
         }
         else
         {
            tmpcnode->GotNetmails++;
            tmpcnode->GotNetmailBytes+=size;
         }
      }
      
      toss_read++;
      
      mm->Flags |= MMFLAG_TOSSED;

      if(!(*handlefunc)(mm))
      {
         osClose(fh);
         mmFree(mm);
         return(FALSE);
      }

      msgoffset=osFTell(fh);

      if(osRead(fh,PktMsgHeader,SIZE_PKTMSGHEADER) < 2)
      {
         LogWrite(1,TOSSINGERR,"Packet header too short for msg #%u (offset %d)",msgnum+1,msgoffset);
         osClose(fh);
         mmFree(mm);
         sprintf(buf,"Packet header too short for msg #%u (offset %d)",msgnum+1,msgoffset);
         BadFile(pkt,buf);
         return(TRUE);
      }
   }

   if(getuword(PktMsgHeader,PKTMSGHEADER_PKTTYPE)!=0)
   {
      osClose(fh);
      mmFree(mm);
      LogWrite(1,TOSSINGERR,"Unknown message type %u for message #%u (offset %d)",getuword(PktMsgHeader,PKTMSGHEADER_PKTTYPE),msgnum+1,msgoffset);
      sprintf(buf,"Unknown message type %u for message #%u (offset %d)",getuword(PktMsgHeader,PKTMSGHEADER_PKTTYPE),msgnum+1,msgoffset);
      BadFile(pkt,buf);
      return(TRUE);
   }

   printf("                      \x0d");
   fflush(stdout);

   osClose(fh);

   return(TRUE);
}

/*************************************************************************/
/*                                                                       */
/*                               Write Pkt                               */
/*                                                                       */
/*************************************************************************/

struct Pkt
{
   struct Pkt *Next;
   osFile fh;
   uint32_t hexnum;
   uint32_t Len;
   uint16_t Type;
   struct Node4D Dest;
   struct Node4D Orig;
};

void pktWrite(struct Pkt *pkt,uint8_t *buf,uint32_t len)
{
   if(!osWrite(pkt->fh,buf,len))
		{ ioerror=TRUE; ioerrornum=osError(); }

   pkt->Len+=len;
}

void WriteNull(struct Pkt *pkt,char *str)
{
   pktWrite(pkt,(uint8_t *)str,(uint32_t)(strlen(str)+1));
}

struct Pkt *FindPkt(struct Node4D *node,struct Node4D *mynode,uint16_t type)
{
   struct Pkt *pkt;

   for(pkt=(struct Pkt *)PktList.First;pkt;pkt=pkt->Next)
      if(Compare4D(node,&pkt->Dest)==0 && Compare4D(mynode,&pkt->Orig)==0 && type==pkt->Type)
         return(pkt);

   return(NULL);
}

time_t lastt;
uint32_t serial;

struct Pkt *CreatePkt(struct Node4D *dest,struct ConfigNode *node,struct Node4D *orig,uint16_t type)
{
   char buf[100],buf2[100];
   struct Pkt *pkt;
   uint32_t num,c;
   time_t t;
   struct tm *tp;
   uint8_t PktHeader[SIZE_PKTHEADER];

   do
   {
      t=time(NULL);
      if(t == lastt) serial++;
      else serial=0;
      if(serial == 256) serial=0;
      lastt=t; 
      num = (t<<8) + serial;
      sprintf(buf2,"%08x.newpkt",num);
      
      MakeFullPath(config.cfg_PacketCreate,buf2,buf,100);
   } while(osExists(buf));

   if(!(pkt=(struct Pkt *)osAllocCleared(sizeof(struct Pkt))))
   {
      nomem=TRUE;
      return(NULL);
   }

   if(!(pkt->fh=osOpen(buf,MODE_NEWFILE)))
   {
		uint32_t err=osError();
      LogWrite(1,SYSTEMERR,"Unable to create packet %s",buf);
		LogWrite(1,SYSTEMERR,"Error: %s",osErrorMsg(err));
      osFree(pkt);
      return(NULL);
   }

   pkt->hexnum=num;
   pkt->Type=type;
   Copy4D(&pkt->Dest,dest);
   Copy4D(&pkt->Orig,orig);

   putuword(PktHeader,PKTHEADER_ORIGNODE,orig->Node);
   putuword(PktHeader,PKTHEADER_DESTNODE,dest->Node);

   time(&t);
   tp=localtime(&t);

   putuword(PktHeader,PKTHEADER_DAY,tp->tm_mday);
   putuword(PktHeader,PKTHEADER_MONTH,tp->tm_mon);
   putuword(PktHeader,PKTHEADER_YEAR,tp->tm_year+1900);
   putuword(PktHeader,PKTHEADER_HOUR,tp->tm_hour);
   putuword(PktHeader,PKTHEADER_MINUTE,tp->tm_min);
   putuword(PktHeader,PKTHEADER_SECOND,tp->tm_sec);

   putuword(PktHeader,PKTHEADER_BAUD,0);
   putuword(PktHeader,PKTHEADER_PKTTYPE,2);
   putuword(PktHeader,PKTHEADER_ORIGNET,orig->Net);
   putuword(PktHeader,PKTHEADER_DESTNET,dest->Net);
   PktHeader[PKTHEADER_PRODCODELOW]=0xfe;
   PktHeader[PKTHEADER_REVMAJOR]=VERSION_MAJOR;
   putuword(PktHeader,PKTHEADER_QORIGZONE,orig->Zone);
   putuword(PktHeader,PKTHEADER_QDESTZONE,dest->Zone);
   putuword(PktHeader,PKTHEADER_AUXNET,0);
   putuword(PktHeader,PKTHEADER_CWVALIDCOPY,0x0100);
   PktHeader[PKTHEADER_PRODCODEHIGH]=0;
   PktHeader[PKTHEADER_REVMINOR]=VERSION_MINOR;
   putuword(PktHeader,PKTHEADER_CAPABILWORD,0x0001);
   putuword(PktHeader,PKTHEADER_ORIGZONE,orig->Zone);
   putuword(PktHeader,PKTHEADER_DESTZONE,dest->Zone);
   putuword(PktHeader,PKTHEADER_ORIGPOINT,orig->Point);
   putuword(PktHeader,PKTHEADER_DESTPOINT,dest->Point);
   PktHeader[PKTHEADER_PRODDATA]=0;
   PktHeader[PKTHEADER_PRODDATA+1]=0;
   PktHeader[PKTHEADER_PRODDATA+2]=0;
   PktHeader[PKTHEADER_PRODDATA+3]=0;

   for(c=0;c<8;c++)
      PktHeader[PKTHEADER_PASSWORD+c]=0;

   if(node)
      strncpy((char *)&PktHeader[PKTHEADER_PASSWORD],node->PacketPW,8);

   pktWrite(pkt,PktHeader,SIZE_PKTHEADER);

   if(ioerror)
   {
      osClose(pkt->fh);
      osFree(pkt);
      return(NULL);
   }

   return(pkt);
}

void FinishPacket(struct Pkt *pkt)
{
   pktWrite(pkt,(uint8_t *)"",1);
   pktWrite(pkt,(uint8_t *)"",1);
   osClose(pkt->fh);

   if(pkt->hexnum)
   {
      char oldname[200],newname[200],buf1[100],buf2[100],*typestr;

      /* Create packet name */

      switch(pkt->Type)
      {
         case PKTS_HOLD:      typestr="Hold";
                              break;
         case PKTS_NORMAL:    typestr="Normal";
                              break;
         case PKTS_DIRECT:    typestr="Direct";
                              break;
         case PKTS_CRASH:     typestr="Crash";
                              break;
         case PKTS_ECHOMAIL:  typestr="Echomail";
                              break;
      }

      sprintf(buf1,"%08x.newpkt",pkt->hexnum);

      sprintf(buf2,"%08x_%s_%d_%d_%d_%d.newpkt",
         pkt->hexnum,
         typestr,
         pkt->Dest.Zone,
         pkt->Dest.Net,
         pkt->Dest.Node,
         pkt->Dest.Point);

      MakeFullPath(config.cfg_PacketCreate,buf1,oldname,200);
      MakeFullPath(config.cfg_PacketCreate,buf2,newname,200);

		if(!osRename(oldname,newname))
		{
			uint32_t err=osError();
			LogWrite(1,SYSTEMERR,"Failed to rename %s to %s",oldname,newname);
			LogWrite(1,SYSTEMERR,"Error: %s",osErrorMsg(err));
		}	
   }
   
   jbFreeNode(&PktList,(struct jbNode *)pkt);
}

void ClosePackets(void)
{
   struct Pkt *pkt,*pkt2;

   pkt=(struct Pkt *)PktList.First;

   while(pkt)
   {
      pkt2=pkt->Next;
      FinishPacket(pkt);
      pkt=pkt2;
   }
}

bool WriteMsgHeader(struct Pkt *pkt,struct MemMessage *mm)
{
   uint8_t PktMsgHeader[SIZE_PKTMSGHEADER];

   putuword(PktMsgHeader,PKTMSGHEADER_PKTTYPE,0x0002);
   putuword(PktMsgHeader,PKTMSGHEADER_ORIGNODE,mm->OrigNode.Node);
   putuword(PktMsgHeader,PKTMSGHEADER_DESTNODE,mm->DestNode.Node);
   putuword(PktMsgHeader,PKTMSGHEADER_ORIGNET,mm->OrigNode.Net);
   putuword(PktMsgHeader,PKTMSGHEADER_DESTNET,mm->DestNode.Net);
   putuword(PktMsgHeader,PKTMSGHEADER_ATTR,mm->Attr);
   putuword(PktMsgHeader,PKTMSGHEADER_COST,mm->Cost);

   pktWrite(pkt,PktMsgHeader,SIZE_PKTMSGHEADER);

   WriteNull(pkt,mm->DateTime);
   WriteNull(pkt,mm->To);
   WriteNull(pkt,mm->From);
   WriteNull(pkt,mm->Subject);

   if(ioerror)
      return(FALSE);

   return(TRUE);
}

bool WritePath(struct Pkt *pkt,struct jbList *list)
{
   uint16_t c;
   struct Path *path;

   for(path=(struct Path *)list->First;path;path=path->Next)
      for(c=0;c<path->Paths;c++)
         if(path->Path[c][0]!=0)
         {
            pktWrite(pkt,(uint8_t *)"\x01PATH: ",7);
            pktWrite(pkt,(uint8_t *)path->Path[c],(uint32_t)strlen(path->Path[c]));
            pktWrite(pkt,(uint8_t *)"\x0d",1);
         }

   if(ioerror)
      return(FALSE);

   return(TRUE);
}

bool WriteSeenBy(struct Pkt *pkt,struct jbList *list)
{
   char *buf;

   if(!(buf=mmMakeSeenByBuf(list)))
      return(FALSE);

   pktWrite(pkt,(uint8_t *)buf,(uint32_t)strlen(buf));

   osFree(buf);

   return(TRUE);
}

bool WriteEchoMail(struct MemMessage *mm,struct ConfigNode *node, struct Aka *aka)
{
   char buf[100];
   struct Node4D *From4D;
   struct Pkt *pkt;
   struct TextChunk  *chunk;
   uint32_t size;

   From4D=&aka->Node;

	if(node->Flags & NODE_PKTFROM)
		From4D=&node->PktFrom;

   toss_written++;

   mm->Type=PKTS_ECHOMAIL;

   size=0;

   for(chunk=(struct TextChunk *)mm->TextChunks.First;chunk;chunk=chunk->Next)
      size+=chunk->Length;

   node->SentEchomails++;
   node->SentEchomailBytes+=size;

   pkt=FindPkt(&node->Node,From4D,mm->Type);

   if(!pkt || (pkt && config.cfg_MaxPktSize!=0 && pkt->Len > config.cfg_MaxPktSize))
   {
      if(pkt) FinishPacket(pkt);

      if(!(pkt=CreatePkt(&node->Node,node,From4D,mm->Type)))
      {
         return(FALSE);
      }

      jbAddNode(&PktList,(struct jbNode *)pkt);
   }

   Copy4D(&mm->DestNode,&node->Node);
   Copy4D(&mm->OrigNode,From4D);

   if(!WriteMsgHeader(pkt,mm))
      return(FALSE);

   sprintf(buf,"AREA:%s\x0d",mm->Area);

   pktWrite(pkt,(uint8_t *)buf,(uint32_t)strlen(buf));

   if(ioerror)
      return(FALSE);

   for(chunk=(struct TextChunk *)mm->TextChunks.First;chunk;chunk=chunk->Next)
   {
      pktWrite(pkt,(uint8_t *)chunk->Data,chunk->Length);
      
      if(ioerror)
         return(FALSE);
   }

   if(node->Node.Zone != aka->Node.Zone || (node->Flags & NODE_TINYSEENBY))
   {
      struct jbList seenbylist;
      struct Nodes2D *seenby;

      if(!(seenby=(struct Nodes2D *)osAlloc(sizeof(struct Nodes2D))))
      {
         nomem=TRUE;
         return(FALSE);
      }

      seenby->Nodes=0;
      seenby->Next=NULL;

      jbNewList(&seenbylist);
      jbAddNode(&seenbylist,(struct jbNode *)seenby);

      if(node->Node.Point == 0)
         if(!mmAddNodes2DList(&seenbylist,node->Node.Net,node->Node.Node))
         {
            jbFreeList(&seenbylist);
            return(FALSE);
         }

      if(aka->Node.Point == 0)
         if(!mmAddNodes2DList(&seenbylist,aka->Node.Net,aka->Node.Node))
         {
            jbFreeList(&seenbylist);
            return(FALSE);
         }

      if(!mmSortNodes2D(&seenbylist))
      {
         jbFreeList(&seenbylist);
         return(FALSE);
      }

      if(!WriteSeenBy(pkt,&seenbylist))
      {
         jbFreeList(&seenbylist);
         return(FALSE);
      }

      jbFreeList(&seenbylist);
   }
   else if(!(node->Flags & NODE_NOSEENBY))
   {
      if(!mmSortNodes2D(&mm->SeenBy))
         return(FALSE);

      if(!WriteSeenBy(pkt,&mm->SeenBy))
         return(FALSE);
   }

   if(!WritePath(pkt,&mm->Path))
      return(FALSE);

   pktWrite(pkt,(uint8_t *)"",1);

   return(TRUE);
}

bool WriteNetMail(struct MemMessage *mm,struct Node4D *dest,struct Aka *aka)
{
   struct Pkt *pkt;
   struct ConfigNode *cnode;
   struct TextChunk *chunk;
  	struct Node4D *From4D;
   uint32_t size;

   toss_written++;

 	From4D=&aka->Node;

   for(cnode=(struct ConfigNode *)config.CNodeList.First;cnode;cnode=cnode->Next)
      if(Compare4D(&cnode->Node,dest)==0) break;

   if(cnode)
   {
      /* Calculate size */

      size=0;

      for(chunk=(struct TextChunk *)mm->TextChunks.First;chunk;chunk=chunk->Next)
         size+=chunk->Length;

      cnode->SentNetmails++;
      cnode->SentNetmailBytes+=size;

      if(cnode->Flags & NODE_PACKNETMAIL)
         if(mm->Type == PKTS_NORMAL) mm->Type=PKTS_ECHOMAIL;

   	if(cnode->Flags & NODE_PKTFROM)
			From4D=&cnode->PktFrom;
   }

   pkt=FindPkt(dest,From4D,mm->Type);

   if(!pkt || (pkt && config.cfg_MaxPktSize!=0 && pkt->Len > config.cfg_MaxPktSize))
   {
      if(pkt) FinishPacket(pkt);

      if(!(pkt=CreatePkt(dest,cnode,From4D,mm->Type)))
      {
         return(FALSE);
      }

      jbAddNode(&PktList,(struct jbNode *)pkt);
   }

   if(!WriteMsgHeader(pkt,mm))
      return(FALSE);

   for(chunk=(struct TextChunk *)mm->TextChunks.First;chunk;chunk=chunk->Next)
   {
      pktWrite(pkt,(uint8_t *)chunk->Data,chunk->Length);

      if(ioerror)
         return(FALSE);
   }

   pktWrite(pkt,(uint8_t *)"",1);

   return(TRUE);
}

