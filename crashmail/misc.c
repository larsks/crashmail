#include "crashmail.h"

void ExpandPacker(uchar *cmd,uchar *dest,uint32_t destsize,uchar *arc,uchar *file)
{
   uint32_t c,d;

   d=0;
   for(c=0;c<strlen(cmd);c++)
   {
      if(cmd[c]=='%' && (cmd[c+1]|32)=='a')
      {
         strncpy(&dest[d],arc,(size_t)(destsize-1-d));
         dest[destsize-1]=0;
         d=strlen(dest);
         c++;
      }
      else if(cmd[c]=='%' && (cmd[c+1]|32)=='f')
      {
         strncpy(&dest[d],file,(size_t)(destsize-1-d));
         dest[destsize-1]=0;
         d=strlen(dest);
         c++;
      }
      else if(cmd[c]=='%' && cmd[c+1]!=0)
      {
         dest[d++]=cmd[c+1];
         c++;
      }
      else dest[d++]=cmd[c];
   }
   dest[d]=0;
}

void ExpandFilter(uchar *cmd,uchar *dest,uint32_t destsize,
   uchar *rfc1,
	uchar *rfc2,
   uchar *msg,
   uchar *area,
   uchar *subj,
   uchar *date,
   uchar *from,
   uchar *to,
   uchar *orignode,
   uchar *destnode)
{
   uint32_t c,d;

   d=0;
   for(c=0;c<strlen(cmd);c++)
   {
      if(cmd[c]=='%' && (cmd[c+1])=='a')
      {
         strncpy(&dest[d],area,(size_t)(destsize-1-d));
         dest[destsize-1]=0;
         d=strlen(dest);
         c++;
      }
      else if(cmd[c]=='%' && (cmd[c+1])=='r')
      {
         strncpy(&dest[d],rfc1,(size_t)(destsize-1-d));
         dest[destsize-1]=0;
         d=strlen(dest);
         c++;
      }
      else if(cmd[c]=='%' && (cmd[c+1])=='R')
      {
         strncpy(&dest[d],rfc2,(size_t)(destsize-1-d));
         dest[destsize-1]=0;
         d=strlen(dest);
         c++;
      }
      else if(cmd[c]=='%' && (cmd[c+1]|32)=='m')
      {
         strncpy(&dest[d],msg,(size_t)(destsize-1-d));
         dest[destsize-1]=0;
         d=strlen(dest);
         c++;
      }
      else if(cmd[c]=='%' && (cmd[c+1]|32)=='s')
      {
         strncpy(&dest[d],subj,(size_t)(destsize-1-d));
         dest[destsize-1]=0;
         d=strlen(dest);
         c++;
      }
      else if(cmd[c]=='%' && (cmd[c+1]|32)=='x')
      {
         strncpy(&dest[d],date,(size_t)(destsize-1-d));
         dest[destsize-1]=0;
         d=strlen(dest);
         c++;
      }
      else if(cmd[c]=='%' && (cmd[c+1]|32)=='f')
      {
         strncpy(&dest[d],from,(size_t)(destsize-1-d));
         dest[destsize-1]=0;
         d=strlen(dest);
         c++;
      }
      else if(cmd[c]=='%' && (cmd[c+1]|32)=='t')
      {
         strncpy(&dest[d],to,(size_t)(destsize-1-d));
         dest[destsize-1]=0;
         d=strlen(dest);
         c++;
      }
      else if(cmd[c]=='%' && (cmd[c+1]|32)=='o')
      {
         strncpy(&dest[d],orignode,(size_t)(destsize-1-d));
         dest[destsize-1]=0;
         d=strlen(dest);
         c++;
      }
      else if(cmd[c]=='%' && (cmd[c+1]|32)=='d')
      {
         strncpy(&dest[d],destnode,(size_t)(destsize-1-d));
         dest[destsize-1]=0;
         d=strlen(dest);
         c++;
      }
      else if(cmd[c]=='%' && cmd[c+1]!=0)
      {
         dest[d++]=cmd[c+1];
         c++;
      }
      else dest[d++]=cmd[c];
   }
   dest[d]=0;
}

int DateCompareFE(const void *f1, const void *f2)
{
   if((*(struct osFileEntry **)f1)->Date > (*(struct osFileEntry  **)f2)->Date)
      return(1);

   if((*(struct osFileEntry **)f1)->Date < (*(struct osFileEntry  **)f2)->Date)
      return(-1);

   return stricmp((*(struct osFileEntry **)f1)->Name,(*(struct osFileEntry **)f2)->Name);

	/* Compares by filenames to get packet files with the same date in the right
		order */
}

bool SortFEList(struct jbList *list)
{
   struct osFileEntry *ftt,**buf,**work;
   uint32_t nodecount = 0;

   for(ftt=(struct osFileEntry *)list->First;ftt;ftt=ftt->Next)
      nodecount++;

   if(!nodecount)
      return(TRUE);

   if(!(buf=(struct osFileEntry **)osAlloc(nodecount * sizeof(struct osFileEntry *))))
   {
      nomem=TRUE;
      return(FALSE);
   }

   work=buf;

   for(ftt=(struct osFileEntry *)list->First;ftt;ftt=ftt->Next)
      *work++=ftt;

   qsort(buf,(size_t)nodecount,(size_t)sizeof(struct osFileEntry *),DateCompareFE);

   jbNewList(list);

   for(work=buf;nodecount--;)
      jbAddNode(list,(struct jbNode *)*work++);

   osFree(buf);

	return(TRUE);
}

bool IsArc(uchar *file)
{
   int c;
   uchar ext[4];

   if(strlen(file)!=12) return(FALSE);
   if(file[8]!='.')     return(FALSE);

   for(c=0;c<8;c++)
      if((file[c]<'0' || file[c]>'9') && ((tolower(file[c]) < 'a') || (tolower(file[c]) > 'f'))) return(FALSE);

   strncpy(ext,&file[9],2);
   ext[2]=0;

   if(stricmp(ext,"MO")==0) return(TRUE);
   if(stricmp(ext,"TU")==0) return(TRUE);
   if(stricmp(ext,"WE")==0) return(TRUE);
   if(stricmp(ext,"TH")==0) return(TRUE);
   if(stricmp(ext,"FR")==0) return(TRUE);
   if(stricmp(ext,"SA")==0) return(TRUE);
   if(stricmp(ext,"SU")==0) return(TRUE);

   return(FALSE);
}

bool IsPkt(uchar *file)
{
   if(strlen(file)!=12)             return(FALSE);
   if(file[8]!='.')                 return(FALSE);
   if(stricmp(&file[9],"pkt")!=0)   return(FALSE);

   return(TRUE);
}

bool IsNewPkt(uchar *file)
{
   if(strlen(file) < 7)
      return(FALSE);

   if(stricmp(&file[strlen(file)-7],".newpkt")!=0)
      return(FALSE);

   return(TRUE);
}

bool IsPktTmp(uchar *file)
{
   if(strlen(file) < 7)
      return(FALSE);

   if(stricmp(&file[strlen(file)-7],".pkttmp")!=0)
      return(FALSE);

   return(TRUE);
}

bool IsOrphan(uchar *file)
{
   if(strlen(file) < 7)
      return(FALSE);

   if(stricmp(&file[strlen(file)-7],".orphan")!=0)
      return(FALSE);

   return(TRUE);
}

bool IsBad(uchar *file)
{
   if(strlen(file)>4 && stricmp(&file[strlen(file)-4],".bad")==0)
      return(TRUE);

   return(FALSE);
}

void striptrail(uchar *str)
{
   int c;

   for(c=strlen(str)-1;str[c] < 33 && c>=0;c--) 
		str[c]=0;
}


void striplead(uchar *str)
{
   int c;

	c=0;
	
	while(str[c]==' ')
		c++;

	strcpy(str,&str[c]);
}

void stripleadtrail(uchar *str)
{
	striplead(str);
	striptrail(str);
}	

void BadFile(uchar *filename,uchar *comment)
{
   uchar destname[100],numbuf[10];
   uint32_t num;

   LogWrite(3,TOSSINGERR,"Renaming %s to .bad",filename);

   num=0;

   do
   {
      MakeFullPath(config.cfg_Inbound,GetFilePart(filename),destname,90);
      strcat(destname,".bad");

      if(num != 0)
      {
         sprintf(numbuf,",%ld",num);
         strcat(destname,numbuf);
      }

      num++;

   } while(osExists(destname));

   if(!movefile(filename,destname))
   {
		uint32_t err=osError();
      LogWrite(1,SYSTEMERR,"Failed to move %s to %s",filename,destname);
		LogWrite(1,SYSTEMERR,"Error: %s",osErrorMsg(err));
      return;
   }

   osSetComment(destname,comment);
}

bool MatchFlags(uchar group,uchar *node)
{
   uchar c;

   for(c=0;c<strlen(node);c++)
   {
      if(group==node[c])
         return(TRUE);
    }

   return(FALSE);
}

bool AddTID(struct MemMessage *mm)
{
   uchar buf[100];

   strcpy(buf,"\x01" "TID: CrashMail II/" OS_PLATFORM_NAME " " TID_VERSION "\x0d");

   return mmAddLine(mm,buf);
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

#define COPYBUFSIZE 5000

bool copyfile(uchar *file,uchar *newfile)
{
   osFile ifh,ofh;
   uint32_t len;
   uchar *copybuf;

   if(!(copybuf=(uchar *)malloc(COPYBUFSIZE)))
	{
		nomem=TRUE;
      return(FALSE);
	}

   if(!(ifh=osOpen(file,MODE_OLDFILE)))
   {
      free(copybuf); 
      return(FALSE);
   }

   if(!(ofh=osOpen(newfile,MODE_NEWFILE)))
   {
      free(copybuf);
      osClose(ifh);
      return(FALSE);
   }

   while((len=osRead(ifh,copybuf,COPYBUFSIZE)) && !ioerror)
	{
      if(!osWrite(ofh,copybuf,len))
			{ ioerror=TRUE; ioerrornum=osError(); }
	}

   free(copybuf);

   osClose(ofh);
   osClose(ifh);

   if(ioerror)
   {
      osDelete(newfile);
      return(FALSE);
   }

   return(TRUE);
}

bool movefile(uchar *file,uchar *newfile)
{
   if(osRename(file,newfile))
      return(TRUE); /* rename was enough */

   if(!copyfile(file,newfile))
      return(FALSE);
 
   osDelete(file);
   return(TRUE);    
}

uchar ChangeType(struct Node4D *dest,uchar pri)
{
   struct Change *change;
   uchar newpri;
   bool ispattern;

   newpri=pri;

   for(change=(struct Change *)config.ChangeList.First;change;change=change->Next)
   {
      if(Compare4DPat(&change->Pattern,dest)==0)
      {
         ispattern=FALSE;

         if(pri == PKTS_ECHOMAIL && change->ChangeNormal == TRUE) ispattern=TRUE;
         if(pri == PKTS_NORMAL   && change->ChangeNormal == TRUE) ispattern=TRUE;
         if(pri == PKTS_CRASH    && change->ChangeCrash  == TRUE) ispattern=TRUE;
         if(pri == PKTS_DIRECT   && change->ChangeDirect == TRUE) ispattern=TRUE;
         if(pri == PKTS_HOLD     && change->ChangeHold   == TRUE) ispattern=TRUE;

         if(ispattern)
         {
            if(pri == PKTS_ECHOMAIL && change->DestPri == PKTS_NORMAL)
               newpri=pri;

            else
               newpri=change->DestPri;
         }
      }
   }

   return(newpri);
}

bool MakeNetmailKludges(struct MemMessage *mm)
{
   uchar buf[100];

   if(mm->OrigNode.Point)
   {
      sprintf(buf,"\x01" "FMPT %u\x0d",mm->OrigNode.Point);
      mmAddBuf(&mm->TextChunks,buf,strlen(buf));
   }
   if(mm->DestNode.Point)
   {
      sprintf(buf,"\x01TOPT %u\x0d",mm->DestNode.Point);
      mmAddBuf(&mm->TextChunks,buf,strlen(buf));
   }
   if(mm->OrigNode.Zone != mm->DestNode.Zone || (config.cfg_Flags & CFG_FORCEINTL))
   {
      sprintf(buf,"\x01INTL %u:%u/%u %u:%u/%u\x0d",
         mm->DestNode.Zone,
         mm->DestNode.Net,
         mm->DestNode.Node,
         mm->OrigNode.Zone,
         mm->OrigNode.Net,
         mm->OrigNode.Node);

      mmAddBuf(&mm->TextChunks,buf,strlen(buf));
   }

	return(TRUE);
}

/*************************************/
/*                                   */
/* Fido DateTime conversion routines */
/*                                   */
/*************************************/

time_t FidoToTime(uchar *date)
{
   uint32_t month;
   struct tm tm;
   time_t t;

   static char *mo[12] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                           "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

   if(date[2]==' ' && date[6]==' ')
   {
      /* Standard Fido                "01 Jan 91  11:22:33"   */

      /* Get month */

      for(month=0;month<12;month++)
         if(strnicmp(mo[month],&date[3],3)==0) break;

      if(month==12)
         return time(NULL); /* return current time */

      tm.tm_sec=atoi(&date[17]);
      tm.tm_min=atoi(&date[14]);
      tm.tm_hour=atoi(&date[11]);
      tm.tm_mday=atoi(date);
      tm.tm_mon=month;
      tm.tm_year=atoi(&date[7]);
      tm.tm_wday=0;
      tm.tm_yday=0;
      tm.tm_isdst=-1;
		
		if(tm.tm_year < 80) /* Y2K fix */
			tm.tm_year+=100;			
   }
   else
   {
      /* SEAdog "Wed 13 Jan 86 02:34" */
      /* SEAdog "Wed  3 Jan 86 02:34" */

      /* Get month */

      for(month=0;month<12;month++)
         if(strnicmp(mo[month],&date[7],3)==0) break;

      if(month==12)
         return time(NULL); /* return current time */

      tm.tm_sec=0;
      tm.tm_min=atoi(&date[17]);
      tm.tm_hour=atoi(&date[14]);
      tm.tm_mday=atoi(&date[4]);
      tm.tm_mon=month;
      tm.tm_year=atoi(&date[11]);
      tm.tm_wday=0;
      tm.tm_yday=0;
      tm.tm_isdst=-1;

		if(tm.tm_year < 80) /* Y2K fix */
			tm.tm_year+=100;			
   }

   t=mktime(&tm);

   if(t == -1)
      t=time(NULL);

   return(t);
}

bool Parse5D(uchar *buf, struct Node4D *n4d, uchar *domain)
{
   uint32_t c=0;
   uchar buf2[100];

   domain[0]=0;

   mystrncpy(buf2,buf,100);

   for(c=0;c<strlen(buf2);c++)
      if(buf2[c]=='@') break;

   if(buf2[c]=='@')
   {
      buf2[c]=0;
      mystrncpy(domain,&buf2[c+1],20);
   }

   return Parse4D(buf2,n4d);
}

bool ExtractAddress(uchar *origin, struct Node4D *n4d)
{
   uint32_t pos,e;
   uchar addr[50];
	uchar domain[20];

   pos=strlen(origin);

   while(pos>0 && origin[pos]!='(') pos--;
      /* Find address */

   if(origin[pos]!='(')
      return(FALSE);

   pos++;

   while(origin[pos]!=13 && (origin[pos]<'0' || origin[pos]>'9')) pos++;
      /* Skip (FidoNet ... ) */

   e=0;

   while(origin[pos]!=')' && e<49)
      addr[e++]=origin[pos++];

   addr[e]=0;

   return Parse5D(addr,n4d,domain);
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

/* WriteRFC and WriteMSG */

void MakeRFCAddr(uchar *dest,uchar *nam,struct Node4D *node,uchar *dom)
{
	uchar domain[50],name[50];
	int j;
	
	/* Prepare domain */

	strcpy(domain,dom);
	
	for(j=0;domain[j];j++)
		domain[j]=tolower(domain[j]);
		
 	if(stricmp(domain,"fidonet")==0)
		strcpy(domain,"fidonet.org");	
		
	/* Prepare name */

	strcpy(name,nam);
	
	for(j=0;name[j];j++)
		if(name[j] == ' ') name[j]='_';

	/* Make addr */
	
	if(node->Point != 0) 
		sprintf(dest,"%s@p%u.f%u.n%u.z%u.%s",
			name,
			node->Point,
			node->Node,
			node->Net,
			node->Zone,
			domain);

	else
		sprintf(dest,"%s@f%u.n%u.z%u.%s",
			name,
			node->Node,
			node->Net,
			node->Zone,
			domain);
}

bool WriteRFC(struct MemMessage *mm,uchar *name,bool rfcaddr)
{
   osFile fh;
   uchar *domain;
   struct Aka *aka;
   struct TextChunk *tmp;
   uint32_t c,d,lastspace;
   uchar buffer[100],fromaddr[100],toaddr[100];

   for(aka=(struct Aka *)config.AkaList.First;aka;aka=aka->Next)
      if(Compare4D(&mm->DestNode,&aka->Node)==0) break;

   domain="FidoNet"; 
 
   if(aka && aka->Domain[0]!=0)
      domain=aka->Domain;

	if(rfcaddr)
	{
		MakeRFCAddr(fromaddr,mm->From,&mm->OrigNode,domain);
		MakeRFCAddr(toaddr,mm->To,&mm->DestNode,domain);
	}
	else
	{
   	sprintf(fromaddr,"%u:%u/%u.%u@%s",mm->OrigNode.Zone,
                                            mm->OrigNode.Net,
                                            mm->OrigNode.Node,
                                            mm->OrigNode.Point,
                                            domain);

   	sprintf(toaddr,"%u:%u/%u.%u@%s",mm->DestNode.Zone,
                                          mm->DestNode.Net,
                                          mm->DestNode.Node,
                                          mm->DestNode.Point,
                                          domain);
	}

   if(!(fh=osOpen(name,MODE_NEWFILE)))
   {
		uint32_t err=osError();
      LogWrite(1,SYSTEMERR,"Unable to write RFC-message to %s",name);
		LogWrite(1,SYSTEMERR,"Error: %s",osErrorMsg(err));
			
      return(FALSE);
   }

   /* Write header */

   if(!osFPrintf(fh,"From: %s (%s)\n",fromaddr,mm->From))
		{ ioerror=TRUE; ioerrornum=osError(); }

   if(!osFPrintf(fh,"To: %s (%s)\n",toaddr,mm->To))
		{ ioerror=TRUE; ioerrornum=osError(); }

   if(!osFPrintf(fh,"Subject: %s\n",mm->Subject))
		{ ioerror=TRUE; ioerrornum=osError(); }

   if(!osFPrintf(fh,"Date: %s\n",mm->DateTime))
		{ ioerror=TRUE; ioerrornum=osError(); }

   if(mm->MSGID[0]!=0)
	{	
      if(!osFPrintf(fh,"Message-ID: <%s>\n",mm->MSGID))
			{ ioerror=TRUE; ioerrornum=osError(); }
	}
	
   if(mm->REPLY[0]!=0)
	{
      if(!osFPrintf(fh,"References: <%s>\n",mm->REPLY))
			{ ioerror=TRUE; ioerrornum=osError(); }
	}
	
   /* Write kludges */

   for(tmp=(struct TextChunk *)mm->TextChunks.First;tmp;tmp=tmp->Next)
   {
      c=0;

      while(c<tmp->Length)
      {
         for(d=c;d<tmp->Length && tmp->Data[d]!=13 && tmp->Data[d]!=10;d++);
         if(tmp->Data[d]==13 || tmp->Data[d]==10) d++;

         if(tmp->Data[c]==1 && d-c-2!=0)
         {
				if(!osPuts(fh,"X-Fido-"))
					{ ioerror=TRUE; ioerrornum=osError(); }

				if(!osWrite(fh,&tmp->Data[c+1],d-c-2))
					{ ioerror=TRUE; ioerrornum=osError(); }

            if(!osPuts(fh,"\n"))
					{ ioerror=TRUE; ioerrornum=osError(); }
         }
         c=d;
      }
   }

   /* Write end-of-header */

   if(!osPuts(fh,"\n"))
		{ ioerror=TRUE; ioerrornum=osError(); }

   /* Write message text */

   for(tmp=(struct TextChunk *)mm->TextChunks.First;tmp;tmp=tmp->Next)
   {
      d=0;

      while(d<tmp->Length)
      {
         lastspace=0;
         c=0;

         while(tmp->Data[d+c]==10) d++;

         while(c<78 && d+c<tmp->Length && tmp->Data[d+c]!=13)
         {
            if(tmp->Data[d+c]==32) lastspace=c;
            c++;
         }

         if(c==78 && lastspace)
         {
            strncpy(buffer,&tmp->Data[d],lastspace);
            buffer[lastspace]=0;
            d+=lastspace+1;
         }
         else
         {
            strncpy(buffer,&tmp->Data[d],c);
            buffer[c]=0;
            if(tmp->Data[d+c]==13) c++;
            d+=c;
         }

         if(buffer[0]!=1)
         {
            if(!osPuts(fh,buffer))
					{ ioerror=TRUE; ioerrornum=osError(); }

            if(!osPutChar(fh,'\n'))
					{ ioerror=TRUE; ioerrornum=osError(); }
         }
      }
   }

   osClose(fh);

   if(ioerror)
      return(FALSE);

	return(TRUE);
}

bool WriteMSG(struct MemMessage *mm,uchar *file)
{
   struct StoredMsg Msg;
   struct TextChunk *chunk;
   struct Path *path;
   osFile fh;
   int c;
   
   strcpy(Msg.From,mm->From);
   strcpy(Msg.To,mm->To);
   strcpy(Msg.Subject,mm->Subject);
   strcpy(Msg.DateTime,mm->DateTime);

   Msg.TimesRead=0;
   Msg.ReplyTo=0;
   Msg.NextReply=0;
   Msg.Cost= mm->Cost;
   Msg.Attr= mm->Attr | FLAG_SENT;

   if(mm->Area[0]==0)
   {
      Msg.DestZone   =  mm->DestNode.Zone;
      Msg.DestNet    =  mm->DestNode.Net;
      Msg.DestNode   =  mm->DestNode.Node;
      Msg.DestPoint  =  mm->DestNode.Point;

      Msg.OrigZone   =  mm->OrigNode.Zone;
      Msg.OrigNet    =  mm->OrigNode.Net;
      Msg.OrigNode   =  mm->OrigNode.Node;
      Msg.OrigPoint  =  mm->OrigNode.Point;
   }
   else
   {
      Msg.DestZone   =  0;
      Msg.DestNet    =  0;
      Msg.DestNode   =  0;
      Msg.DestPoint  =  0;

      Msg.OrigZone   =  0;
      Msg.OrigNet    =  0;
      Msg.OrigNode   =  0;
      Msg.OrigPoint  =  0;
   }

   if(!(fh=osOpen(file,MODE_NEWFILE)))
   {
		uint32_t err=osError();
      LogWrite(1,SYSTEMERR,"Unable to write message to %s",file);
		LogWrite(1,SYSTEMERR,"Error: %s",osErrorMsg(err));

      return(FALSE);
   }

   /* Write header */

	if(!osWrite(fh,&Msg,sizeof(struct StoredMsg)))
		{ ioerror=TRUE; ioerrornum=osError(); }

   /* Write text */

   for(chunk=(struct TextChunk *)mm->TextChunks.First;chunk;chunk=chunk->Next)
	{
      if(!osWrite(fh,chunk->Data,chunk->Length))
			{ ioerror=TRUE; ioerrornum=osError(); }
	}	

   /* Write seen-by */

   if((config.cfg_Flags & CFG_IMPORTSEENBY) && mm->Area[0]!=0)
   {
      uchar *sbbuf;

      if(!(sbbuf=mmMakeSeenByBuf(&mm->SeenBy)))
      {
         osClose(fh);
         return(FALSE);
      }

      if(sbbuf[0])
		{
         if(!osWrite(fh,sbbuf,(uint32_t)strlen(sbbuf)))
				{ ioerror=TRUE; ioerrornum=osError(); }
		}	

      osFree(sbbuf);
   }

   /* Write path */

   for(path=(struct Path *)mm->Path.First;path;path=path->Next)
      for(c=0;c<path->Paths;c++)
         if(path->Path[c][0]!=0)
         {
				if(!osWrite(fh,"\x01PATH: ",7))
					{ ioerror=TRUE; ioerrornum=osError(); }
				
            if(!osWrite(fh,path->Path[c],(uint32_t)strlen(path->Path[c])))
					{ ioerror=TRUE; ioerrornum=osError(); }

            if(!osWrite(fh,"\x0d",1))
					{ ioerror=TRUE; ioerrornum=osError(); }
         }

   if(!osPutChar(fh,0))
		{ ioerror=TRUE; ioerrornum=osError(); }

   osClose(fh);

   if(ioerror)
      return(FALSE);

   return(TRUE);
}
