#include "crashmail.h"

/* Boyer-Moore search routines */

uint16_t bmstep[256];

void bminit(char *pat)
{
   int i,len;

   len=strlen(pat);

   for(i=0;i<256;i++)
      bmstep[i]=0;

   for(i=0;i<len;i++)
      bmstep[tolower(pat[i])]=len-i-1;

   for(i=0;i<256;i++)
      if(bmstep[i] == 0) bmstep[i]=len;
}

long bmfind(char *pat,char *text,long textlen,long start)
{
   long i,j,end,len;

   len=strlen(pat);
   end=textlen-len;

   i=start;

   while(i<=end)
   {
      for(j=len-1;j>=0;j--)
         if(tolower(text[i+j])!=tolower(pat[j])) break;

      if(j == -1)
         return(i);

      i+=bmstep[tolower(text[i+j])];
   }

   return(-1);
}

struct MemMessage *filter_mm;

int filter_nllookup(struct Node4D *node)
{
   if(!config.cfg_NodelistType)
      return(-1);

   if(!nodelistopen)
      return(1);

   return (*config.cfg_NodelistType->nlCheckNode)(node);
}

int filter_comparenode(char *var,struct Node4D *node,char *operator,char *data,int opn,int datan,int *errpos,char **errstr)
{
   static char errbuf[100];
   struct Node4DPat pat;

   if(operator[0]==0)
   {
      sprintf(errbuf,"%s is not a boolean variable",var);
      *errstr=errbuf;
      *errpos=opn;
      return(-1);
   }

   if(strcmp(operator,"=")!=0)
   {
      sprintf(errbuf,"%s is not a valid operator for %s",operator,var);
      *errstr=errbuf;
      *errpos=opn;
      return(-1);
   }

   if(!(Parse4DPat(data,&pat)))
   {
      sprintf(errbuf,"Invalid node pattern \"%s\"",data);
      *errstr=errbuf;
      *errpos=datan;
      return(-1);
   }

   if(!config.cfg_NodelistType && !Check4DPatNodelist(&pat))
   {
      sprintf(errbuf,"Nodelist needed for pattern \"%s\"",data);
      *errstr=errbuf;
      *errpos=datan;
      return(-1);
   }

   if(Compare4DPat(&pat,node)==0)
      return(1);

   return(0);
}

int filter_comparestring(char *var,char *str,char *operator,char *data,int opn,int datan,int *errpos,char **errstr)
{
   static char errbuf[100];

   if(operator[0]==0)
   {
      sprintf(errbuf,"%s is not a boolean variable",var);
      *errstr=errbuf;
      *errpos=opn;
      return(-1);
   }

   if(strcmp(operator,"|")==0)
   {
      bminit(data);

      if(bmfind(data,str,strlen(str),0) != -1)
         return(1);

      return(0);
   }
   else if(strcmp(operator,"=")==0)
   {
      if(!(osCheckPattern(data)))
      {
         sprintf(errbuf,"Invalid pattern \"%s\"",data);
         *errstr=errbuf;
         *errpos=datan;
         return(-1);
      }

      if(osMatchPattern(data,str))
         return(1);

      return(0);
   }
   else
   {
      sprintf(errbuf,"%s is not a valid operator for %s",operator,var);
      *errstr=errbuf;
      *errpos=opn;
      return(-1);
   }
}

int filter_comparebool(char *var,bool bl,char *operator,char *data,int opn,int datan,int *errpos,char **errstr)
{
   static char errbuf[100];

   if(operator[0]==0)
   {
      if(bl)
         return(1);

      return(0);
   }
   else if(strcmp(operator,"=")==0)
   {
      if(stricmp(data,"TRUE")==0)
      {
         if(bl)
            return(1);

         return(0);
      }
      else if(stricmp(data,"FALSE")==0)
      {
         if(bl)
            return(0);

         return(1);
      }
      else
      {
         sprintf(errbuf,"Boolean variable %s can only be TRUE or FALSE",var);
         *errstr=errbuf;
         *errpos=datan;
         return(-1);
      }
   }
   else
   {
      sprintf(errbuf,"%s is not a valid operator for %s",operator,var);
      *errstr=errbuf;
      *errpos=opn;
      return(-1);
   }
}

int filter_comparetext(char *var,struct MemMessage *mm,bool kludge,char *operator,char *data,int opn,int datan,int *errpos,char **errstr)
{
   static char errbuf[100];
   struct TextChunk *chunk;
   long start,pos,c;

   if(operator[0]==0)
   {
      sprintf(errbuf,"%s is not a boolean variable",var);
      *errstr=errbuf;
      *errpos=opn;
      return(-1);
   }

   if(strcmp(operator,"|")!=0)
   {
      sprintf(errbuf,"%s is not a valid operator for %s",operator,var);
      *errstr=errbuf;
      *errpos=opn;
      return(-1);
   }

   bminit(data);

   for(chunk=(struct TextChunk *)mm->TextChunks.First;chunk;chunk=chunk->Next)
   {
      /* Search chunk */

      start=0;

      while((pos=bmfind(data,chunk->Data,chunk->Length,start)) != -1)
      {
         start=pos+1;

         for(c=pos;c>0 && chunk->Data[c]!=1 && chunk->Data[c]!=13;c--);

         if(chunk->Data[c]==1 && kludge)  return(1);
         if(chunk->Data[c]!=1 && !kludge) return(1);
      }
   }

   return(0);
}

int filter_evalfunc(char *str,int *errpos,char **errstr)
{
   char type[20],source[20];
   static char errbuf[100];
   struct Aka *aka;
   struct ConfigNode *cnode;
   bool fileattach,tolocalaka,fromlocalaka,tolocalpoint,fromlocalpoint;
   bool existscfg_fromaddr,existscfg_fromboss,existscfg_toaddr,existscfg_toboss;
   char var[100],operator[2],data[100];
   int opn,datan,res;
   int c,d;
   struct Node4D from4d,fromboss,toboss;

   /* Parse statement */

   c=0;
   d=0;

   while(str[c]!=0 && (isalpha(str[c]) || str[c] == '_') && d<99)
      var[d++]=str[c++];

   var[d]=0;

   opn=c;

   if(str[c]!=0)
   {
      operator[0]=str[c++];
      operator[1]=0;
   }
   else
   {
      operator[0]=0;
   }

   datan=c;

   if(str[c]=='"')
      c++;

   mystrncpy(data,&str[c],100);

   if(data[0]!=0)
   {
      if(data[strlen(data)-1]=='"')
         data[strlen(data)-1]=0;
   }

	/* Set real fromaddr and fromboss, toboss */

   if(filter_mm->Area[0] == 0) Copy4D(&from4d,&filter_mm->OrigNode);
   else                        Copy4D(&from4d,&filter_mm->Origin4D);

   Copy4D(&fromboss,&from4d);
   fromboss.Point=0;

   Copy4D(&toboss,&filter_mm->DestNode);
   toboss.Point=0;

   /* Make local variables */

   tolocalaka=FALSE;
   fromlocalaka=FALSE;
   tolocalpoint=FALSE;
   fromlocalpoint=FALSE;

   existscfg_fromaddr=FALSE;
   existscfg_fromboss=FALSE;
   existscfg_toaddr=FALSE;
   existscfg_toboss=FALSE;

   fileattach=FALSE;

   for(aka=(struct Aka *)config.AkaList.First;aka;aka=aka->Next)
   {
      if(Compare4D(&filter_mm->DestNode,&aka->Node)==0) tolocalaka=TRUE;
      if(Compare4D(&from4d,&aka->Node)==0)              fromlocalaka=TRUE;

      if(filter_mm->DestNode.Point != 0)
         if(Compare4D(&aka->Node,&toboss)==0 && aka->Node.Point==0) tolocalpoint=TRUE;

      if(from4d.Point != 0)
         if(Compare4D(&aka->Node,&fromboss)==0 && aka->Node.Point==0) fromlocalpoint=TRUE;
   }

   for(cnode=(struct ConfigNode *)config.CNodeList.First;cnode;cnode=cnode->Next)
   {
      if(Compare4D(&cnode->Node,&from4d)==0)   existscfg_fromaddr=TRUE;
      if(Compare4D(&cnode->Node,&fromboss)==0) existscfg_fromboss=TRUE;

      if(Compare4D(&cnode->Node,&filter_mm->DestNode)==0) existscfg_toaddr=TRUE;
      if(Compare4D(&cnode->Node,&toboss)==0)              existscfg_toboss=TRUE;
   }

   if(filter_mm->Attr & FLAG_FILEATTACH)
      fileattach=TRUE;

   if(filter_mm->Area[0]==0)
      strcpy(type,"NETMAIL");

   else
      strcpy(type,"ECHOMAIL");

   strcpy(source,"OTHER");

   if(filter_mm->Flags & MMFLAG_EXPORTED)
	   strcpy(source,"EXPORTED");

  	if(filter_mm->Flags & MMFLAG_AUTOGEN)
      strcpy(source,"CRASHMAIL");

  	if(filter_mm->Flags & MMFLAG_TOSSED)
		strcpy(source,"TOSSED");

   /* Compare */

   if(stricmp(var,"FROMADDR")==0)
      return filter_comparenode("FROMADDR",&from4d,operator,data,opn,datan,errpos,errstr);

   if(stricmp(var,"TOADDR")==0)
      return filter_comparenode("TOADDR",&filter_mm->DestNode,operator,data,opn,datan,errpos,errstr);

   if(stricmp(var,"FROMNAME")==0)
      return filter_comparestring("FROMNAME",filter_mm->From,operator,data,opn,datan,errpos,errstr);

   if(stricmp(var,"TONAME")==0)
      return filter_comparestring("TONAME",filter_mm->To,operator,data,opn,datan,errpos,errstr);

   if(stricmp(var,"SUBJECT")==0)
      return filter_comparestring("SUBJECT",filter_mm->Subject,operator,data,opn,datan,errpos,errstr);

   if(stricmp(var,"AREA")==0)
      return filter_comparestring("AREA",filter_mm->Area,operator,data,opn,datan,errpos,errstr);

   if(stricmp(var,"TYPE")==0)
      return filter_comparestring("TYPE",type,operator,data,opn,datan,errpos,errstr);

   if(stricmp(var,"SOURCE")==0)
      return filter_comparestring("SOURCE",source,operator,data,opn,datan,errpos,errstr);

   if(stricmp(var,"FILEATTACH")==0)
      return filter_comparebool("FILEATTACH",fileattach,operator,data,opn,datan,errpos,errstr);

   if(stricmp(var,"TOLOCALAKA")==0)
      return filter_comparebool("TOLOCALAKA",tolocalaka,operator,data,opn,datan,errpos,errstr);

   if(stricmp(var,"FROMLOCALAKA")==0)
      return filter_comparebool("FROMLOCALAKA",fromlocalaka,operator,data,opn,datan,errpos,errstr);

   if(stricmp(var,"TOLOCALPOINT")==0)
      return filter_comparebool("TOLOCALPOINT",tolocalpoint,operator,data,opn,datan,errpos,errstr);

   if(stricmp(var,"FROMLOCALPOINT")==0)
      return filter_comparebool("FROMLOCALPOINT",fromlocalpoint,operator,data,opn,datan,errpos,errstr);

   if(stricmp(var,"EXISTSCFG_FROMADDR")==0)
      return filter_comparebool("EXISTSCFG_FROMADDR",existscfg_fromaddr,operator,data,opn,datan,errpos,errstr);

   if(stricmp(var,"EXISTSCFG_FROMBOSS")==0)
      return filter_comparebool("EXISTSCFG_FROMBOSS",existscfg_fromboss,operator,data,opn,datan,errpos,errstr);

   if(stricmp(var,"EXISTSCFG_TOADDR")==0)
      return filter_comparebool("EXISTSCFG_TOADDR",existscfg_toaddr,operator,data,opn,datan,errpos,errstr);

   if(stricmp(var,"EXISTSCFG_TOBOSS")==0)
      return filter_comparebool("EXISTSCFG_TOBOSS",existscfg_toboss,operator,data,opn,datan,errpos,errstr);

   if(stricmp(var,"EXISTSNL_FROMADDR")==0)
   {
      res=filter_nllookup(&from4d);

      if(res == -1)
      {
         sprintf(errbuf,"Nodelist required for variable %s",var);
         *errstr=errbuf;
         *errpos=0;

         return(-1);
      }

      return res;
   }

   if(stricmp(var,"EXISTSNL_FROMBOSS")==0)
   {
      res=filter_nllookup(&fromboss);

      if(res == -1)
      {
         sprintf(errbuf,"Nodelist required for variable %s",var);
         *errstr=errbuf;
         *errpos=0;

         return(-1);
      }

      return res;
   }

   if(stricmp(var,"EXISTSNL_TOADDR")==0)
   {
      res=filter_nllookup(&filter_mm->DestNode);

      if(res == -1)
      {
         sprintf(errbuf,"Nodelist required for variable %s",var);
         *errstr=errbuf;
         *errpos=0;

         return(-1);
      }

      return res;
   }

   if(stricmp(var,"EXISTSNL_TOBOSS")==0)
   {
      res=filter_nllookup(&toboss);

      if(res == -1)
      {
         sprintf(errbuf,"Nodelist required for variable %s",var);
         *errstr=errbuf;
         *errpos=0;

         return(-1);
      }

      return res;
   }

   if(stricmp(var,"EXISTSCFG_FROMBOSS")==0)
      return filter_comparebool("EXISTSCFG_FROMBOSS",existscfg_fromboss,operator,data,opn,datan,errpos,errstr);

   if(stricmp(var,"EXISTSCFG_TOADDR")==0)
      return filter_comparebool("EXISTSCFG_TOADDR",existscfg_toaddr,operator,data,opn,datan,errpos,errstr);

   if(stricmp(var,"EXISTSCFG_TOBOSS")==0)
      return filter_comparebool("EXISTSCFG_TOBOSS",existscfg_toboss,operator,data,opn,datan,errpos,errstr);

   if(stricmp(var,"TEXT")==0)
      return filter_comparetext("TEXT",filter_mm,FALSE,operator,data,opn,datan,errpos,errstr);

   if(stricmp(var,"KLUDGES")==0)
      return filter_comparetext("KLUDGES",filter_mm,TRUE,operator,data,opn,datan,errpos,errstr);

   sprintf(errbuf,"Unknown variable %s",var);
   *errstr=errbuf;
   *errpos=0;

   return(-1);
}


bool Filter_Kill(struct MemMessage *mm)
{
   char buf[200];

	if(mm->Area[0] == 0)
	{
		Print4D(&mm->OrigNode,buf);

	   LogWrite(4,TOSSINGINFO,"Filter: Killing netmail from \"%s\" at %s",
			mm->From,
			buf);
	}
	else
	{
	   LogWrite(4,TOSSINGINFO,"Filter: Killing message from \"%s\" in %s",
			mm->From,
			mm->Area);
	}

	mm->Flags |= MMFLAG_KILL;

	return(TRUE);
}

bool Filter_Twit(struct MemMessage *mm)
{
   char buf[200];

	if(mm->Area[0] == 0)
	{
		Print4D(&mm->OrigNode,buf);

	   LogWrite(4,TOSSINGINFO,"Filter: Twitting netmail from \"%s\" at %s",
         mm->From,
			buf);
	}
	else
	{
	   LogWrite(4,TOSSINGINFO,"Filter: Twitting message from \"%s\" in %s",
			mm->From,
			mm->Area);
	}

	mm->Flags |= MMFLAG_TWIT;

	return(TRUE);
}

bool Filter_Copy(struct MemMessage *mm,char *tagname)
{
   struct Area *area;
   struct TextChunk *tmp;
   struct jbList oldlist;
   char buf[200];

   LogWrite(4,TOSSINGINFO,"Filter: Copying message to area %s",tagname);

   for(area=(struct Area *)config.AreaList.First;area;area=area->Next)
      if(stricmp(area->Tagname,tagname)==0) break;

   if(!area)
      return(TRUE); /* We have already checked this in CheckConfig(), the area should exist */

	oldlist.First=mm->TextChunks.First;
	oldlist.Last=mm->TextChunks.Last;

	jbNewList(&mm->TextChunks);

   if(mm->Area[0])
   {
      sprintf(buf,"AREA:%s\x0d",mm->Area);
      mmAddBuf(&mm->TextChunks,buf,strlen(buf));
   }

   for(tmp=(struct TextChunk *)oldlist.First;tmp;tmp=tmp->Next)
      mmAddBuf(&mm->TextChunks,tmp->Data,tmp->Length);

   if(!((*area->Messagebase->importfunc)(mm,area)))
      return(FALSE);

   area->NewTexts++;

   jbFreeList(&mm->TextChunks);

   mm->TextChunks.First=oldlist.First;
   mm->TextChunks.Last=oldlist.Last;

   return(TRUE);
}

bool Filter_WriteBad(struct MemMessage *mm,char *reason)
{
   LogWrite(4,TOSSINGINFO,"Filter: Writing message to BAD area (\"%s\")",reason);

   return WriteBad(mm,reason);
}

bool Filter_WriteLog(struct MemMessage *mm,char *cmd)
{
   char buf[400];
   char origbuf[30],destbuf[30];

   if(mm->Area[0] == 0)
      Print4D(&mm->OrigNode,origbuf);

   else
      Print4D(&mm->Origin4D,origbuf);

   Print4D(&mm->DestNode,destbuf);

   ExpandFilter(cmd,buf,400,
 	   "",
      "",
      "",
		mm->Area,
      mm->Subject,
      mm->DateTime,
      mm->From,
      mm->To,
      origbuf,
      destbuf);

   LogWrite(1,TOSSINGINFO,"%s",buf);

   return(TRUE);
}

bool Filter_Execute(struct MemMessage *mm,char *cmd)
{
   bool msg,rfc1,rfc2;
   char msgbuf[L_tmpnam],rfcbuf1[L_tmpnam],rfcbuf2[L_tmpnam];
   char origbuf[30],destbuf[30];
   char buf[400];
	int arcres;

   msg=FALSE;
   rfc1=FALSE;
	rfc2=FALSE;

   msgbuf[0]=0;
   rfcbuf1[0]=0;
   rfcbuf2[0]=0;

   if(strstr(cmd,"%m")) msg=TRUE;
   if(strstr(cmd,"%r")) rfc1=TRUE;
   if(strstr(cmd,"%R")) rfc2=TRUE;

   if(rfc1) tmpnam(rfcbuf1);
   if(rfc2) tmpnam(rfcbuf2);
   if(msg)  tmpnam(msgbuf);

   if(mm->Area[0] == 0)
      Print4D(&mm->OrigNode,origbuf);

   else
      Print4D(&mm->Origin4D,origbuf);

   Print4D(&mm->DestNode,destbuf);

   ExpandFilter(cmd,buf,400,
 	   rfcbuf1,
      rfcbuf2,
      msgbuf,
		mm->Area,
      mm->Subject,
      mm->DateTime,
      mm->From,
      mm->To,
      origbuf,
      destbuf);

   if(rfc1) WriteRFC(mm,rfcbuf1,FALSE);
	if(rfc2) WriteRFC(mm,rfcbuf2,TRUE);
   if(msg)  WriteMSG(mm,msgbuf);

   LogWrite(4,SYSTEMINFO,"Filter: Executing external command \"%s\"",buf);

   arcres=osExecute(buf);

   if(rfc1) osDelete(rfcbuf1);
   if(rfc2) osDelete(rfcbuf2);
   if(msg) osDelete(msgbuf);

   if(arcres == 0)
   {
   	/* Command ok */

	   LogWrite(1,SYSTEMERR,"Filter: External command returned without error, killing message\n");
		mm->Flags |= MMFLAG_KILL;
      return(TRUE);
   }

   if(arcres >= 20)
   {
	   LogWrite(1,SYSTEMERR,"Filter: External command failed with error %u, exiting...",arcres);
      return(FALSE);
   }

	return(TRUE);
}

bool Filter_Bounce(struct MemMessage *mm,char *reason,bool headeronly)
{
   char reasonbuf[400];
   char origbuf[30],destbuf[30];

   if(mm->Area[0] == 0)
      Print4D(&mm->OrigNode,origbuf);

   else
      Print4D(&mm->Origin4D,origbuf);

   Print4D(&mm->DestNode,destbuf);

   ExpandFilter(reason,reasonbuf,400,
 	   "",
      "",
      "",
		mm->Area,
      mm->Subject,
      mm->DateTime,
      mm->From,
      mm->To,
      origbuf,
      destbuf);

   LogWrite(4,TOSSINGINFO,"Filter: Bouncing message (\"%s\")",reasonbuf);

   return Bounce(mm,reasonbuf,headeronly);
}

bool Filter_Remap(struct MemMessage *mm,char *namepat,struct Node4DPat *destpat)
{
   struct Route *tmproute;
   struct jbList oldlist;
   struct TextChunk *tmp;
   char buf[100];
   char oldto[36],newto[36];
   struct Node4D olddest4d,newdest4d,my4d;
   uint32_t c,d;
	bool skip;

   if(mm->Area[0])
   {
      LogWrite(1,SYSTEMERR,"Filter: Only netmails can be remapped");
      return(TRUE);
   }

   strcpy(oldto,mm->To);
   Copy4D(&olddest4d,&mm->DestNode);

   ExpandNodePat(destpat,&mm->DestNode,&newdest4d);

   if(strcmp(namepat,"*")==0) strcpy(newto,mm->To);
   else                       strcpy(newto,namepat);

   my4d.Zone=0;
   my4d.Net=0;
   my4d.Node=0;
   my4d.Point=0;

   for(tmproute=(struct Route *)config.RouteList.First;tmproute;tmproute=tmproute->Next)
      if(Compare4DPat(&tmproute->Pattern,&newdest4d)==0) break;

   if(tmproute)
      Copy4D(&my4d,&tmproute->Aka->Node);

   LogWrite(4,SYSTEMINFO,"Filter: Remapping message to %s at %u:%u/%u.%u",
      newto,
      newdest4d.Zone,
      newdest4d.Net,
      newdest4d.Node,
      newdest4d.Point);

   LogWrite(4,SYSTEMINFO,"Filter: Message originally to %s at %u:%u/%u.%u",
      oldto,
      olddest4d.Zone,
      olddest4d.Net,
      olddest4d.Node,
      olddest4d.Point);

   oldlist.First=mm->TextChunks.First;
	oldlist.Last=mm->TextChunks.Last;

	jbNewList(&mm->TextChunks);

	Copy4D(&mm->DestNode,&newdest4d);
   strcpy(mm->To,newto);

   MakeNetmailKludges(mm);

   for(tmp=(struct TextChunk *)oldlist.First;tmp;tmp=tmp->Next)
   {
      c=0;

      while(c<tmp->Length)
      {
         for(d=c;d<tmp->Length && tmp->Data[d]!=13;d++);
         if(tmp->Data[d]==13) d++;

			skip=FALSE;

			if(d-c > 5)
			{
         	if(strncmp(&tmp->Data[c],"\x01""INTL",5)==0) skip=TRUE;
         	if(strncmp(&tmp->Data[c],"\x01""FMPT",5)==0) skip=TRUE;
         	if(strncmp(&tmp->Data[c],"\x01""TOPT",5)==0) skip=TRUE;
         }

         if(d-c!=0 && !skip)
				mmAddBuf(&mm->TextChunks,&tmp->Data[c],d-c);

         c=d;
      }
   }

   sprintf(buf,"\x01Remapped to %s at %u:%u/%u.%u by %u:%u/%u.%u\x0d",
      newto,
      newdest4d.Zone,
      newdest4d.Net,
      newdest4d.Node,
      newdest4d.Point,
      my4d.Zone,
      my4d.Net,
      my4d.Node,
      my4d.Point);

	mmAddLine(mm,buf);

   sprintf(buf,"\x01Message originally to %s at %u:%u/%u.%ud",
      oldto,
      olddest4d.Zone,
      olddest4d.Net,
      olddest4d.Node,
      olddest4d.Point);

	mmAddLine(mm,buf);

   jbFreeList(&oldlist);

	if(nomem)
		return(FALSE);

	return(TRUE);
}

bool Filter(struct MemMessage *mm)
{
   struct Filter *filter;
	struct Command *command;
   struct expr *expr;
   char *errstr;
   int errpos,res;

   for(filter=(struct Filter *)config.FilterList.First;filter;filter=filter->Next)
   {
      if(!(expr=expr_makeexpr(filter->Filter)))
      {
         nomem=TRUE;
         return(FALSE);
      }

      filter_mm=mm;
      res=expr_eval(expr,filter_evalfunc,&errpos,&errstr);
      expr_free(expr);

      if(res == -1)
      {
         /* Error. All these should be caught in config.c/CheckConfig() */

         LogWrite(3,TOSSINGERR,"Syntax error in filter");
         return(TRUE);
      }

      if(res == 1)
      {
			/* Matches filter */

			for(command=(struct Command *)filter->CommandList.First;command;command=command->Next)
			{
				switch(command->Cmd)
				{
					case COMMAND_KILL:
						Filter_Kill(mm);
						break;

					case COMMAND_TWIT:
						Filter_Twit(mm);
						break;

					case COMMAND_COPY:
						Filter_Copy(mm,command->string);
						break;

               case COMMAND_EXECUTE:
                  if(!Filter_Execute(mm,command->string))
                     return(FALSE);

                  break;

               case COMMAND_WRITELOG:
                  if(!Filter_WriteLog(mm,command->string))
							return(FALSE);

                  break;

               case COMMAND_WRITEBAD:
                  if(!Filter_WriteBad(mm,command->string))
							return(FALSE);

						break;

               case COMMAND_BOUNCEMSG:
                  if(!Filter_Bounce(mm,command->string,FALSE))
							return(FALSE);

						break;

               case COMMAND_BOUNCEHEADER:
                  if(!Filter_Bounce(mm,command->string,TRUE))
							return(FALSE);

						break;

               case COMMAND_REMAPMSG:
                  if(!Filter_Remap(mm,command->string,&command->n4ddestpat))
							return(FALSE);

						break;
				}

				if(mm->Flags & MMFLAG_KILL)
					return(TRUE);
			}
      }
   }

   return(TRUE);
}

bool CheckFilter(char *filter,char *cfgerr)
{
   struct expr *expr;
   char *errstr;
   int errpos,res;
   struct MemMessage *mm;
   int c;

   if(!(expr=expr_makeexpr(filter)))
   {
      nomem=TRUE;
      return(FALSE);
   }

   if(!(mm=mmAlloc()))
   {
      expr_free(expr);
      return(FALSE);
   }

   filter_mm=mm;
   res=expr_eval(expr,filter_evalfunc,&errpos,&errstr);
   expr_free(expr);

   mmFree(mm);

   if(res == -1)
   {
		if(strlen(filter) > 200)
		{
			strcpy(cfgerr,"Syntax error in filter");
		}
		else
		{
	      sprintf(cfgerr," Syntax error in filter:\n  %s\n",filter);

   	   for(c=0;c<errpos+2;c++)
      	   strcat(cfgerr," ");

	      strcat(cfgerr,"^");
   	   strcat(cfgerr,"\n");
         strcat(cfgerr," ");
         strcat(cfgerr,errstr);

	      return(FALSE);
		}
   }

   return(TRUE);
}
