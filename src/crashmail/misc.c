#include "crashmail.h"

void ExpandPacker(uchar *cmd,uchar *dest,ulong destsize,uchar *arc,uchar *file)
{
   ulong c,d;

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

void ExpandRobot(uchar *cmd,uchar *dest,ulong destsize,
   uchar *rfc,
   uchar *msg,
   uchar *subj,
   uchar *date,
   uchar *from,
   uchar *to,
   uchar *orignode,
   uchar *destnode)
{
   ulong c,d;

   d=0;
   for(c=0;c<strlen(cmd);c++)
   {
      if(cmd[c]=='%' && (cmd[c+1]|32)=='r')
      {
         strncpy(&dest[d],rfc,(size_t)(destsize-1-d));
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
   ulong nodecount = 0;

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
   int c;

   if(strlen(file)!=12)             return(FALSE);
   if(file[8]!='.')                 return(FALSE);
   if(stricmp(&file[9],"pkt")!=0)   return(FALSE);

   for(c=0;c<8;c++)
      if((file[c]<'0' || file[c]>'9') && ((file[c]|32) < 'a' || (file[c]|32) > 'f')) return(FALSE);

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

void strip(uchar *str)
{
   int c;

   for(c=strlen(str)-1;str[c] < 33 && c>=0;c--) str[c]=0;
}

void BadFile(uchar *filename,uchar *comment)
{
   uchar destname[100],numbuf[10];
   ulong num;

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

   if(!MoveFile(filename,destname))
   {
      LogWrite(1,SYSTEMERR,"Failed to move %s to %s",filename,destname);
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

bool CopyFile(uchar *file,uchar *newfile)
{
   osFile ifh,ofh;
   ulong len;
   uchar *copybuf;

   if(!(copybuf=(uchar *)malloc(COPYBUFSIZE)))
      return(FALSE);

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

   while((len=osRead(ifh,copybuf,COPYBUFSIZE)) && !diskfull)
      osWrite(ofh,copybuf,len);

   free(copybuf);

   osClose(ofh);
   osClose(ifh);

   if(diskfull)
   {
      remove(newfile);
      return(FALSE);
   }

   return(TRUE);
}

bool MoveFile(uchar *file,uchar *newfile)
{
   if(rename(file,newfile)==0)
      return(TRUE); /* rename was enough */

   if(!CopyFile(file,newfile))
      return(FALSE);
 
   remove(file);
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
   ulong month;
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
   }

   t=mktime(&tm);

   if(t == -1)
      t=time(NULL);

   return(t);
}

bool ExtractAddress(uchar *origin, struct Node4D *n4d,uchar *domain)
{
   ulong pos,e;
   uchar addr[50];

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









