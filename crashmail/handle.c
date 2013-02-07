#include "crashmail.h"

bool HandleEchomail(struct MemMessage *mm);
bool HandleNetmail(struct MemMessage *mm);
bool HandleRescan(struct MemMessage *mm);

bool HandleMessage(struct MemMessage *mm)
{
   bool res;

   LogWrite(6,DEBUG,"Is in HandleMessage()");

   handle_nesting++;

   if(mm->Area[0]==0) res=HandleNetmail(mm);
   else               res=HandleEchomail(mm);

   handle_nesting--;

   return(res);
}

/**************************** auto-add *****************************/

bool GetDescription(uchar *area,struct ConfigNode *node,uchar *desc)
{
   struct Arealist *arealist;
   uchar buf[200];
   ulong c,d;
   osFile fh;

   for(arealist=(struct Arealist *)config.ArealistList.First;arealist;arealist=arealist->Next)
   {
      if(arealist->Node == node && (arealist->Flags & AREALIST_DESC))
      {
         if((fh=osOpen(arealist->AreaFile,MODE_OLDFILE)))
         {
            while(osFGets(fh,buf,199))
            {
               for(c=0;buf[c]>32;c++);

               if(buf[c]!=0)
               {
                  buf[c]=0;

                  if(stricmp(buf,area)==0)
                  {
                     c++;
                     while(buf[c]<=32 && buf[c]!=0) c++;

                     if(buf[c]!=0)
                     {
                        d=0;
                        while(buf[c]!=0 && buf[c]!=10 && buf[c]!=13 && d<77) desc[d++]=buf[c++];
                        desc[d]=0;
                        osClose(fh);
                        return(TRUE);
                     }
                  }
               }
            }
            osClose(fh);
         }
			else
			{
				ulong err=osError();
				LogWrite(1,SYSTEMERR,"Failed to open file \"%s\"\n",arealist->AreaFile);
				LogWrite(1,SYSTEMERR,"Error: %s",osErrorMsg(err));
			}
      }
   }
   return(FALSE);
}

bool AddTossNode(struct Area *area,struct ConfigNode *cnode,ushort flags)
{
   struct TossNode *tnode;

   /* Check if it already exists */

   for(tnode=(struct TossNode *)area->TossNodes.First;tnode;tnode=tnode->Next)
      if(tnode->ConfigNode == cnode) return(TRUE);

   if(!(tnode=(struct TossNode *)osAllocCleared(sizeof(struct TossNode))))
   {
      nomem=TRUE;
      return(FALSE);
   }

   jbAddNode((struct jbList *)&area->TossNodes,(struct jbNode *)tnode),
   tnode->ConfigNode=cnode;
   tnode->Flags=flags;

   return(TRUE);
}

time_t lastt;

void MakeDirectory(uchar *dest,ulong destsize,uchar *defdir,uchar *areaname)
{
   ulong c,d;
   uchar lowercase[200],shortname[50];

   /* Convert to lower case */

   strcpy(lowercase,areaname);

   for(c=0;lowercase[c]!=0;c++)
      lowercase[c]=tolower(lowercase[c]);

   /* Make 8 digit serial number */
  
   if(lastt == 0) lastt=time(NULL);
   else lastt++;

   sprintf(shortname,"%08lx",lastt);

   d=0;
   for(c=0;c<strlen(defdir) && d!=destsize-1;c++)
   {
      if(defdir[c]=='%' && (defdir[c+1]|32)=='a')
      {
         strncpy(&dest[d],areaname,(size_t)(destsize-1-d));
         dest[destsize-1]=0;
         d=strlen(dest);
         c++;
      }
      else if(defdir[c]=='%' && (defdir[c+1]|32)=='l')
      {
         strncpy(&dest[d],lowercase,(size_t)(destsize-1-d));
         dest[destsize-1]=0;
         d=strlen(dest);
         c++;
      }
      else if(defdir[c]=='%' && (defdir[c+1]|32)=='8')
      {
         strncpy(&dest[d],shortname,(size_t)(destsize-1-d));
         dest[destsize-1]=0;
         d=strlen(dest);
         c++;
      }
      else dest[d++]=defdir[c];
   }
   dest[d]=0;
}

struct Area *AddArea(uchar *name,struct Node4D *node,struct Node4D *mynode,ulong active,ulong forcepassthru)
{
   struct Area *temparea,*defarea;
   struct Aka *tempaka;
   struct ConfigNode *tempcnode;

   if(!(temparea=(struct Area *)osAllocCleared(sizeof(struct Area))))
   {
      nomem=TRUE;
      return(NULL);
   }

   jbNewList(&temparea->TossNodes);
   jbNewList(&temparea->BannedNodes);

   jbAddNode(&config.AreaList,(struct jbNode *)temparea);

   for(tempaka=(struct Aka *)config.AkaList.First;tempaka;tempaka=tempaka->Next)
      if(Compare4D(&tempaka->Node,mynode)==0) break;

   if(!tempaka)
      tempaka=(struct Aka *)config.AkaList.First;

   for(tempcnode=(struct ConfigNode *)config.CNodeList.First;tempcnode;tempcnode=tempcnode->Next)
      if(Compare4D(&tempcnode->Node,node)==0) break;

   /* Find default area to use */

   defarea=NULL;

   /* First we try to find one for specific groups */

   if(tempcnode && tempcnode->DefaultGroup)
   {
      uchar groups[100];

      for(defarea=(struct Area *)config.AreaList.First;defarea;defarea=defarea->Next)
         if(strnicmp(defarea->Tagname,"DEFAULT_",8)==0)
         {
            mystrncpy(groups,&defarea->Tagname[8],50);

            if(MatchFlags(tempcnode->DefaultGroup,groups))
               break;
         }
   }

   /* If not found, we try to find the general default area */

   if(!defarea)
   {
      for(defarea=(struct Area *)config.AreaList.First;defarea;defarea=defarea->Next)
         if(stricmp(defarea->Tagname,"DEFAULT")==0) break;
   }

   if(defarea)
   {
      struct TossNode *tnode;
      ulong c;
      uchar *forbiddenchars="\"#'`()*,./:;<>|";
      uchar buf[100],buf2[100];

      strcpy(buf,name);

      for(c=0;buf[c]!=0;c++)
         if(buf[c]<33 || buf[c]>126 || strchr(forbiddenchars,buf[c]))
            buf[c]='_';

      /* Cannot create directory directly into temparea->Path.
         MakeDirectory checks for duplicate area names in the AreaList
         and would get confused */
      MakeDirectory(buf2,80,defarea->Path,buf);
      strcpy(temparea->Path,buf2);

      if(!forcepassthru)
         temparea->Messagebase=defarea->Messagebase;

      strcpy(temparea->Description,defarea->Description);

      if(defarea->Flags & AREA_MANDATORY)
         temparea->Flags |= AREA_MANDATORY;

      if(defarea->Flags & AREA_DEFREADONLY)
         temparea->Flags |= AREA_DEFREADONLY;

      if(defarea->Flags & AREA_IGNOREDUPES)
         temparea->Flags |= AREA_IGNOREDUPES;

      if(defarea->Flags & AREA_IGNORESEENBY)
         temparea->Flags |= AREA_IGNORESEENBY;

      temparea->KeepDays=defarea->KeepDays;
      temparea->KeepNum=defarea->KeepNum;

      for(tnode=(struct TossNode *)defarea->TossNodes.First;tnode;tnode=tnode->Next)
         AddTossNode(temparea,tnode->ConfigNode,tnode->Flags);
   }

   GetDescription(name,tempcnode,temparea->Description);

   if(!active)
      temparea->Flags=AREA_UNCONFIRMED;

   strcpy(temparea->Tagname,name);

   temparea->Aka=tempaka;
	temparea->AreaType = AREATYPE_ECHOMAIL;
	
   if(tempcnode)
   {
      temparea->Group=tempcnode->DefaultGroup;
      AddTossNode(temparea,tempcnode,TOSSNODE_FEED);

      for(tempcnode=(struct ConfigNode *)config.CNodeList.First;tempcnode;tempcnode=tempcnode->Next)
         if(MatchFlags(temparea->Group,tempcnode->AddGroups))
         {
            ushort flags;

            flags=0;

            if((temparea->Flags & AREA_DEFREADONLY) || MatchFlags(temparea->Group,tempcnode->ReadOnlyGroups))
               flags=TOSSNODE_READONLY;

            AddTossNode(temparea,tempcnode,flags);
         }
   }

   config.changed=TRUE;
   temparea->changed=TRUE;

   return(temparea);
}

/**************************** Echomail *****************************/

bool FindNodes2D(struct jbList *list,struct Node4D *node)
{
   struct Nodes2D *tmp;
   ushort c;

   for(tmp=(struct Nodes2D *)list->First;tmp;tmp=tmp->Next)
      for(c=0;c<tmp->Nodes;c++)
         if(tmp->Net[c]==node->Net && tmp->Node[c]==node->Node) return(TRUE);

   return(FALSE);
}

bool WriteBad(struct MemMessage *mm,uchar *reason)
{
   struct Area *temparea;
   struct TextChunk *chunk;

   for(temparea=(struct Area *)config.AreaList.First;temparea;temparea=temparea->Next)
      if(temparea->AreaType == AREATYPE_BAD) break;

   if(!temparea)
   {
      LogWrite(2,TOSSINGERR,"No BAD area configured, message lost");
      return(TRUE);
   }

   /* Insert a new textchunk with information first in the message */

   if(!(chunk=(struct TextChunk *)osAlloc(sizeof(struct TextChunk))))
   {
      nomem=TRUE;
      return(FALSE);
   }

   chunk->Next=(struct TextChunk *)mm->TextChunks.First;
   mm->TextChunks.First = (struct jbNode *)chunk;
   if(!mm->TextChunks.Last) mm->TextChunks.Last=(struct jbNode *)chunk;

   if(mm->Area[0]==0)
   {
      sprintf(chunk->Data,"DEST:%u:%u/%u.%u\x0d"
                          "ORIG:%u:%u/%u.%u\x0d"
                          "PKTORIG:%u:%u/%u.%u\x0d"
                          "PKTDEST:%u:%u/%u.%u\x0d"
                          "ERROR:%s\x0d",
         mm->DestNode.Zone,
         mm->DestNode.Net,
         mm->DestNode.Node,
         mm->DestNode.Point,
         mm->OrigNode.Zone,
         mm->OrigNode.Net,
         mm->OrigNode.Node,
         mm->OrigNode.Point,
         mm->PktOrig.Zone,
         mm->PktOrig.Net,
         mm->PktOrig.Node,
         mm->PktOrig.Point,
         mm->PktDest.Zone,
         mm->PktDest.Net,
         mm->PktDest.Node,
         mm->PktDest.Point,
         reason);

      chunk->Length=strlen(chunk->Data);
   }
   else
   {
      sprintf(chunk->Data,"AREA:%s\x0d"
                          "PKTORIG:%u:%u/%u.%u\x0d"
                          "PKTDEST:%u:%u/%u.%u\x0d"
                          "ERROR:%s\x0d",
         mm->Area,
         mm->PktOrig.Zone,
         mm->PktOrig.Net,
         mm->PktOrig.Node,
         mm->PktOrig.Point,
         mm->PktDest.Zone,
         mm->PktDest.Net,
         mm->PktDest.Node,
         mm->PktDest.Point,
         reason);

      chunk->Length=strlen(chunk->Data);
   }

   if(temparea->Messagebase)
   {
      if(!((*temparea->Messagebase->importfunc)(mm,temparea)))
         return(FALSE);
   }

   /* Remove first chunk again */

   chunk=(struct TextChunk *)mm->TextChunks.First;

   mm->TextChunks.First=(struct jbNode *)chunk->Next;
   if((struct TextChunk *)mm->TextChunks.Last == chunk) mm->TextChunks.Last = NULL;

   osFree(chunk);

   temparea->NewTexts++;

	return(TRUE);
}

bool AddNodePath(struct jbList *list,struct Node4D *node)
{
   uchar buf[40],buf2[10];
   struct Path *path;
   struct Node4D n4d;
   ushort lastnet,num;
   bool lastok;
   ulong jbcpos;

   lastok=FALSE;
   lastnet=0;

   /* Find last entry in Path */

   path=(struct Path *)list->Last;
   if(path && path->Paths!=0)
   {
      num=path->Paths-1;
      jbcpos=0;

      while(jbstrcpy(buf,path->Path[num],40,&jbcpos))
      {
         if(Parse4D(buf,&n4d))
         {
            if(n4d.Net == 0) n4d.Net=lastnet;
            else             lastnet=n4d.Net;
            lastok=TRUE;
         }
         else
         {
            lastok=FALSE;
         }
      }
   }

	/* Are we already in the PATH line? */

	if(lastok)
	{
		if(n4d.Net == node->Net && n4d.Node == node->Node && n4d.Point == node->Point)
			return(TRUE);	
	}

   /* Make address */

   if(lastok && n4d.Net == node->Net)
      sprintf(buf,"%u",node->Node);

   else
      sprintf(buf,"%u/%u",node->Net,node->Node);

   if(node->Point != 0)
   {
      sprintf(buf2,".%u",node->Point);
      strcat(buf,buf2);
   }

   /* Add new */

   path=(struct Path *)list->Last;

   if(path)
   {
      if(path->Paths != 0)
      {
         if(strlen(buf)+strlen(path->Path[path->Paths-1])<=70)
         {
            /* Add to old path */

            strcat(path->Path[path->Paths-1]," ");
            strcat(path->Path[path->Paths-1],buf);
            return(TRUE);
         }
      }
   }

   if(path && path->Paths == PKT_NUMPATH)
      path=NULL; /* Chunk is full */

   if(!path)
   {
      /* Alloc new path */

      if(!(path=(struct Path *)osAlloc(sizeof(struct Path))))
      {
         nomem=TRUE;
         return(FALSE);
      }

      jbAddNode(list,(struct jbNode *)path);
      path->Next=NULL;
      path->Paths=0;
   }

   /* Always net/node when a new line */

   sprintf(path->Path[path->Paths],"%u/%u",node->Net,node->Node);

   if(node->Point != 0)
   {
      sprintf(buf2,".%u",node->Point);
      strcat(path->Path[path->Paths],buf2);
   }

   path->Paths++;

   return(TRUE);
}

uchar *StripRe(uchar *str)
{
   for (;;)
   {
      if(strnicmp (str, "Re:", 3)==0)
      {
         str += 3;
         if (*str == ' ') str++;
      }
      else if(strnicmp (str, "Re^", 3)==0 && str[4]==':')
      {
         str += 5;
         if (*str == ' ') str++;
      }
      else if(strnicmp (str, "Re[", 3)==0 && str[4]==']' && str[5]==':')
      {
         str += 6;
         if (*str == ' ') str++;
      }
      else break;
   }
   return (str);
}

bool HandleEchomail(struct MemMessage *mm)
{
   struct Area *temparea;
   struct TossNode *temptnode;
   struct AddNode  *tmpaddnode;
   struct RemNode  *tmpremnode;
   struct ConfigNode *tempcnode;

   mm->Type=PKTS_ECHOMAIL;

   /* Find orignode */

   for(tempcnode=(struct ConfigNode *)config.CNodeList.First;tempcnode;tempcnode=tempcnode->Next)
      if(Compare4D(&mm->PktOrig,&tempcnode->Node)==0) break;

   /* Find area */

   for(temparea=(struct Area *)config.AreaList.First;temparea;temparea=temparea->Next)
      if(stricmp(temparea->Tagname,mm->Area)==0) break;

   /* Auto-add */

   if(!temparea)
   {
      if(tempcnode)
         temparea=AddArea(mm->Area,&mm->PktOrig,&mm->PktDest,tempcnode->Flags & NODE_AUTOADD,FALSE);

      else
         temparea=AddArea(mm->Area,&mm->PktOrig,&mm->PktDest,FALSE,FALSE);

      if(!temparea)
         return(FALSE);

      if(temparea->Flags & AREA_UNCONFIRMED)
         LogWrite(3,TOSSINGERR,"Unknown area %s",mm->Area);

      else
         LogWrite(3,TOSSINGINFO,"Unknown area %s -- auto-adding",mm->Area);
   }

   /* Don't toss in auto-added areas */

   if(temparea->Flags & AREA_UNCONFIRMED)
   {
      toss_bad++;
      
      if(!WriteBad(mm,"Unknown area (auto-added but not confirmed)"))
         return(FALSE);

      return(TRUE);
   }

   /* Check if the node receives this area */

   if(!(mm->Flags & MMFLAG_EXPORTED) && !(mm->Flags & MMFLAG_NOSECURITY))
   {
      for(temptnode=(struct TossNode *)temparea->TossNodes.First;temptnode;temptnode=temptnode->Next)
         if(Compare4D(&temptnode->ConfigNode->Node,&mm->PktOrig)==0) break;

      if(!temptnode)
      {
         LogWrite(1,TOSSINGERR,"%lu:%lu/%lu.%lu doesn't receive %s",
            mm->PktOrig.Zone,
            mm->PktOrig.Net,
            mm->PktOrig.Node,
            mm->PktOrig.Point,
            mm->Area);

         toss_bad++;

         if(!WriteBad(mm,"Node does not receive this area"))
            return(FALSE);

         return(TRUE);
      }

      if(temptnode->Flags & TOSSNODE_READONLY)
      {
         LogWrite(1,TOSSINGERR,"%lu:%lu/%lu.%lu is not allowed to write in %s",
            mm->PktOrig.Zone,
            mm->PktOrig.Net,
            mm->PktOrig.Node,
            mm->PktOrig.Point,
            mm->Area);

         toss_bad++;
         
         if(!WriteBad(mm,"Node is not allowed to write in this area"))
            return(FALSE);
            
         return(TRUE);
      }
   }

   /* Remove all seen-by:s if the message comes from an other zone */

   if(temparea->Aka->Node.Zone != mm->PktOrig.Zone && mm->SeenBy.First)
      jbFreeList(&mm->SeenBy);

   /* Check if a node already is in seen-by */

   if((config.cfg_Flags & CFG_CHECKSEENBY) && !(temparea->Flags & AREA_IGNORESEENBY))
   {
      for(temptnode=(struct TossNode *)temparea->TossNodes.First;temptnode;temptnode=temptnode->Next)
      {
         temptnode->ConfigNode->IsInSeenBy=FALSE;

         if(temptnode->ConfigNode->Node.Zone == temparea->Aka->Node.Zone)
            if(temptnode->ConfigNode->Node.Point==0)
               if(FindNodes2D(&mm->SeenBy,&temptnode->ConfigNode->Node))
                  temptnode->ConfigNode->IsInSeenBy=TRUE;
      }
   }

   /* Add nodes to seen-by */

   for(temptnode=(struct TossNode *)temparea->TossNodes.First;temptnode;temptnode=temptnode->Next)
      if(temptnode->ConfigNode->Node.Point == 0 && temparea->Aka->Node.Zone == temptnode->ConfigNode->Node.Zone)
         if(!(temptnode->ConfigNode->Flags & NODE_PASSIVE))
            if(!(temptnode->Flags & TOSSNODE_WRITEONLY))
            {
               if(!mmAddNodes2DList(&mm->SeenBy,temptnode->ConfigNode->Node.Net,temptnode->ConfigNode->Node.Node))
                  return(FALSE);
            }

   /* Remove nodes specified in config from seen-by and path */

   for(tmpremnode=(struct RemNode *)temparea->Aka->RemList.First;tmpremnode;tmpremnode=tmpremnode->Next)
      mmRemNodes2DListPat(&mm->SeenBy,&tmpremnode->NodePat);

   /* Add nodes specified in config to seen-by */

   for(tmpaddnode=(struct AddNode *)temparea->Aka->AddList.First;tmpaddnode;tmpaddnode=tmpaddnode->Next)
      mmAddNodes2DList(&mm->SeenBy,tmpaddnode->Node.Net,tmpaddnode->Node.Node);

   /* Add own node to seen-by */

   if(temparea->Aka->Node.Point == 0)
   {
      if(!mmAddNodes2DList(&mm->SeenBy,temparea->Aka->Node.Net,temparea->Aka->Node.Node))
         return(FALSE);
   }

   /* Add own node to path */

   if(temparea->Aka->Node.Point == 0 || (config.cfg_Flags & CFG_PATH3D))
   {
      if(!AddNodePath(&mm->Path,&temparea->Aka->Node))
         return(FALSE);
   }

   /* Dupecheck */

   if(config.cfg_DupeMode!=DUPE_IGNORE && (mm->Flags & MMFLAG_TOSSED) && !(temparea->Flags & AREA_IGNOREDUPES))
   {
      if(CheckDupe(mm))
      {
         LogWrite(4,TOSSINGERR,"Duplicate message in %s",mm->Area);

         toss_dupes++;
         temparea->NewDupes++;

         if(tempcnode)
            tempcnode->Dupes++;

         if(config.cfg_DupeMode == DUPE_BAD)
         {
            if(!WriteBad(mm,"Duplicate message"))
               return(FALSE);
         }

         return(TRUE);
      }
   }

   if(!mmSortNodes2D(&mm->SeenBy))
      return(FALSE);

	/* Filter */

	if(!Filter(mm))
      return(FALSE);

	if(mm->Flags & MMFLAG_KILL)
		return(TRUE);

   temparea->NewTexts++;

   /* Write to all nodes */

   if(!(mm->Flags & MMFLAG_RESCANNED))
   {
      /* not rescanned */
      for(temptnode=(struct TossNode *)temparea->TossNodes.First;temptnode;temptnode=temptnode->Next)
         /* is not sender of packet */
         if(Compare4D(&mm->PktOrig,&temptnode->ConfigNode->Node)!=0)
            /* is not passive */
            if(!(temptnode->ConfigNode->Flags & NODE_PASSIVE))
               /* is not write-only */
               if(!(temptnode->Flags & TOSSNODE_WRITEONLY))
                  /* is not already in seen-by */
                  if(!(temptnode->ConfigNode->IsInSeenBy == TRUE && (config.cfg_Flags & CFG_CHECKSEENBY)))
                  {
                     if(!WriteEchoMail(mm,temptnode->ConfigNode,temparea->Aka))
                        return(FALSE);
                  }
   }

	if(mm->Flags & MMFLAG_TWIT)
		return(TRUE);

   if(!(mm->Flags & MMFLAG_EXPORTED) && temparea->Messagebase)
   {
      toss_import++;

      if(config.cfg_Flags & CFG_STRIPRE)
         strcpy(mm->Subject,StripRe(mm->Subject));

		/* Remove LOCAL flag if set and set SENT flag */

		mm->Attr |= FLAG_SENT;
		mm->Attr &= ~(FLAG_LOCAL);

      if(!(*temparea->Messagebase->importfunc)(mm,temparea))
			return(FALSE);
   }

   return(TRUE);
}

/******************************* netmail **********************************/

/* For loop-mail checking */

bool CheckFoundAka(uchar *str)
{
   struct Node4D via4d;
   struct Aka *aka;

   if(!(strstr(str,":") && strstr(str,"/")))
      return(FALSE);

   if(!Parse4D(str,&via4d))
      return(FALSE);

   if(via4d.Zone==0 || via4d.Net==0)
      return(FALSE);

   for(aka=(struct Aka *)config.AkaList.First;aka;aka=aka->Next)
      if(Compare4D(&aka->Node,&via4d)==0) return(TRUE);

   return(FALSE);
}

bool IsLoopMail(struct MemMessage *mm)
{
   struct TextChunk *tmp;
   ushort q;
   ulong c,d;
   
   for(tmp=(struct TextChunk *)mm->TextChunks.First;tmp;tmp=tmp->Next)
   {
      c=0;

      while(c<tmp->Length)
      {
         for(d=c;d<tmp->Length && tmp->Data[d]!=13;d++);
         if(tmp->Data[d]==13) d++;

         if(strncmp(&tmp->Data[c],"\x01Via",4)==0)
         {
            /* Is ^aVia line */

            uchar via[200];

            if(d-c<150) q=d-c;
            else        q=150;

            strncpy(via,&tmp->Data[c],q);
            via[q]=0;

            if(strstr(via,"CrashMail"))
            {
               /* Is created by CrashMail */

               uchar destbuf[20];
               ushort u,v;

               v=0;

               for(u=0;via[u]!=0;u++)
               {
                  if(via[u]==':' || via[u]=='/' || via[u]=='.' || (via[u]>='0' && via[u]<='9'))
                  {
                     if(v<19) destbuf[v++]=via[u];
                  }
                  else
                  {
                     if(v!=0)
                     {
                        destbuf[v]=0;
                        if(CheckFoundAka(destbuf)) return(TRUE);
                     }
                     v=0;
                  }
               }

               if(v!=0)
               {
                  destbuf[v]=0;
                  if(CheckFoundAka(destbuf)) return(TRUE);
               }
            }
         }
         c=d;
      }
   }

   return(FALSE);
}


/* Bouncing and receipts */

bool Bounce(struct MemMessage *mm,uchar *reason,bool headeronly)
{
   uchar buf[400],*tmpbuf;
   ulong c;
   struct Route *tmproute;
   struct MemMessage *tmpmm;
   struct TextChunk *chunk;
   struct Node4D n4d;

   if(mm->Flags & MMFLAG_AUTOGEN)
   {
      LogWrite(1,TOSSINGERR,"No bounce messages sent for messages created by CrashMail");
      return(TRUE);
   }

   if(mm->Area[0] == 0) Copy4D(&n4d,&mm->OrigNode);
   else                 Copy4D(&n4d,&mm->Origin4D);

   for(tmproute=(struct Route *)config.RouteList.First;tmproute;tmproute=tmproute->Next)
      if(Compare4DPat(&tmproute->Pattern,&n4d)==0) break;

   if(!tmproute)
   {
      Print4D(&n4d,buf);
      LogWrite(1,TOSSINGERR,"Can't bounce message, no routing for %s",buf);
      return(TRUE);
   }

   if(!(tmpmm=mmAlloc()))
      return(FALSE);

   Copy4D(&tmpmm->DestNode,&n4d);
   Copy4D(&tmpmm->OrigNode,&tmproute->Aka->Node);

   strcpy(tmpmm->To,mm->From);
   strcpy(tmpmm->From,config.cfg_Sysop);
   strcpy(tmpmm->Subject,"Bounced message");

   tmpmm->Attr=FLAG_PVT;

   MakeFidoDate(time(NULL),tmpmm->DateTime);

   tmpmm->Flags |= MMFLAG_AUTOGEN;

   MakeNetmailKludges(tmpmm);

   if(config.cfg_Flags & CFG_ADDTID)
      AddTID(tmpmm);

   strncpy(buf,reason,390);
   strcat(buf,"\x0d\x0d");

   mmAddLine(tmpmm,buf);

   if(headeronly) mmAddLine(tmpmm,"This is the header of the message that was bounced:\x0d\x0d");
   else           mmAddLine(tmpmm,"This is the message that was bounced:\x0d\x0d");

   if(mm->Area[0])
   {
      sprintf(buf,"Area: %.80s\x0d",mm->Area);
      mmAddLine(tmpmm,buf);

      sprintf(buf,"From: %.40s (%u:%u/%u.%u)\x0d",mm->From,
         n4d.Zone,
         n4d.Net,
         n4d.Node,
         n4d.Point);

      mmAddLine(tmpmm,buf);

      sprintf(buf,"  To: %.40s\x0d",mm->To);

      mmAddLine(tmpmm,buf);
   }
   else
   {
      sprintf(buf,"From: %.40s (%u:%u/%u.%u)\x0d",mm->From,
         n4d.Zone,
         n4d.Net,
         n4d.Node,
         n4d.Point);

      mmAddLine(tmpmm,buf);

      sprintf(buf,"  To: %.40s (%u:%u/%u.%u)\x0d",
         mm->To,mm->DestNode.Zone,
         mm->DestNode.Net,
         mm->DestNode.Node,
         mm->DestNode.Point);

      mmAddLine(tmpmm,buf);
   }

   sprintf(buf,"Subj: %s\x0d",mm->Subject);

   mmAddLine(tmpmm,buf);

   sprintf(buf,"Date: %s\x0d\x0d",mm->DateTime);

   mmAddLine(tmpmm,buf);

   if(!headeronly)
   {
      for(chunk=(struct TextChunk *)mm->TextChunks.First;chunk;chunk=chunk->Next)
      {
         if(chunk->Length)
         {
         	if(!(tmpbuf=(uchar *)osAlloc(chunk->Length)))
         	{
         	   nomem=TRUE;
               mmFree(tmpmm);
         	   return(FALSE);
      	   }

            for(c=0;c<chunk->Length;c++)
            {
               if(chunk->Data[c]==1) tmpbuf[c]='@';
               else                  tmpbuf[c]=chunk->Data[c];
            }

            mmAddBuf(&tmpmm->TextChunks,tmpbuf,chunk->Length);

            osFree(tmpbuf);
         }
      }
   }

   if(!HandleMessage(tmpmm))
   {
      mmFree(tmpmm);
      return(FALSE);
   }

   mmFree(tmpmm);

   return(TRUE);
}

bool AnswerReceipt(struct MemMessage *mm)
{
   uchar buf[400];
   struct Route *tmproute;
   struct MemMessage *tmpmm;

   LogWrite(4,TOSSINGINFO,"Answering to a receipt request");

   for(tmproute=(struct Route *)config.RouteList.First;tmproute;tmproute=tmproute->Next)
      if(Compare4DPat(&tmproute->Pattern,&mm->OrigNode)==0) break;

   if(!tmproute)
   {
      Print4D(&mm->OrigNode,buf);
      LogWrite(1,TOSSINGERR,"Can't send receipt, no routing for %s",buf);
      return(TRUE);
   }

   if(!(tmpmm=mmAlloc()))
      return(FALSE);

   Copy4D(&tmpmm->DestNode,&mm->OrigNode);
   Copy4D(&tmpmm->OrigNode,&tmproute->Aka->Node);

   strcpy(tmpmm->To,mm->From);
   strcpy(tmpmm->From,config.cfg_Sysop);
   strcpy(tmpmm->Subject,"Receipt");

   tmpmm->Attr=FLAG_PVT | FLAG_IRRR;

   MakeFidoDate(time(NULL),tmpmm->DateTime);

   mm->Flags |= MMFLAG_AUTOGEN;

   MakeNetmailKludges(tmpmm);

   if(config.cfg_Flags & CFG_ADDTID)
      AddTID(tmpmm);

    sprintf(buf,"Your message to %s dated %s with the subject \"%s\" has reached its final "
               "destination. This message doesn't mean that the message has been read, it "
               "just tells you that it has arrived at this system.\x0d\x0d",
               mm->To,mm->DateTime,mm->Subject);

   mmAddLine(tmpmm,buf);

   if(!HandleMessage(tmpmm))
   {
      mmFree(tmpmm);
      return(FALSE);
   }

   mmFree(tmpmm);
   return(TRUE);
}

bool AnswerAudit(struct MemMessage *mm)
{
   uchar buf[200],auditbuf[500];
   struct Route *tmproute,*destroute;
   struct MemMessage *tmpmm;
   struct Node4D n4d;
   
   LogWrite(4,TOSSINGINFO,"Answering to an audit request");

   for(tmproute=(struct Route *)config.RouteList.First;tmproute;tmproute=tmproute->Next)
      if(Compare4DPat(&tmproute->Pattern,&mm->OrigNode)==0) break;

   if(!tmproute)
   {
      Print4D(&mm->OrigNode,buf);
      LogWrite(1,TOSSINGERR,"Can't send  receipt, no routing for %s",buf);
      return(TRUE);
   }

   if(!(tmpmm=mmAlloc()))
      return(FALSE);

   Copy4D(&tmpmm->DestNode,&mm->OrigNode);
   Copy4D(&tmpmm->OrigNode,&tmproute->Aka->Node);

   strcpy(tmpmm->To,mm->From);
   strcpy(tmpmm->From,config.cfg_Sysop);
   strcpy(tmpmm->Subject,"Audit");

   tmpmm->Attr=FLAG_PVT;

   MakeFidoDate(time(NULL),tmpmm->DateTime);

   tmpmm->Flags |= MMFLAG_AUTOGEN;

   MakeNetmailKludges(tmpmm);

   if(config.cfg_Flags & CFG_ADDTID)
      AddTID(tmpmm);

   for(destroute=(struct Route *)config.RouteList.First;destroute;destroute=destroute->Next)
      if(Compare4DPat(&destroute->Pattern,&mm->DestNode)==0) break;

   if(destroute)
   {
      ExpandNodePat(&destroute->DestPat,&mm->DestNode,&n4d);

      sprintf(auditbuf,"Your message to %s dated %s with the subject \"%s\" has just been "
                       "routed to %u:%u/%u.%u by this system.\x0d\x0d",
                        mm->To,mm->DateTime,mm->Subject,
                       n4d.Zone,n4d.Net,n4d.Node,n4d.Point);
   }
   else
   {
      Copy4D(&n4d,&mm->DestNode);
      
      sprintf(auditbuf,"Your message to %s dated %s with the subject \"%s\" could not be "
                       "routed since no routing for %u:%u/%u.%u was configured at this "
                       "system. Message lost! \x0d\x0d",
                       mm->To,mm->DateTime,mm->Subject,
                       n4d.Zone,n4d.Net,n4d.Node,n4d.Point);
   }

   mmAddLine(tmpmm,auditbuf);
   
   if(!HandleMessage(tmpmm))
   {
      mmFree(tmpmm);
      return(FALSE);
   }

   mmFree(tmpmm);

   return(TRUE);
}


/* Main netmail handling */

bool HandleNetmail(struct MemMessage *mm)
{
   struct Area       *tmparea;
   struct Aka        *aka;
   struct ConfigNode *cnode,*pktcnode;
   struct ImportNode *inode;
   struct Route      *tmproute;
	struct Node4D     n4d,Dest4D;
   struct PatternNode *patternnode;
   struct AreaFixName *areafixname;
   struct TextChunk *tmpchunk,*chunk;
   bool istext;
   uchar buf[400],buf2[200],buf3[200],subjtemp[80];
   ulong c,d,jbcpos;
   time_t t;
   struct tm *tp;
   ulong size;
	uchar oldtype;
   bool headeronly;

   /* Find orignode */

   for(pktcnode=(struct ConfigNode *)config.CNodeList.First;pktcnode;pktcnode=pktcnode->Next)
      if(Compare4D(&mm->PktOrig,&pktcnode->Node)==0) break;

   /* Calculate size */

   size=0;

   for(tmpchunk=(struct TextChunk *)mm->TextChunks.First;tmpchunk;tmpchunk=tmpchunk->Next)
      size+=tmpchunk->Length;

   /* Statistics */

   if((mm->Flags & MMFLAG_TOSSED) && pktcnode)
   {
      pktcnode->GotNetmails++;
      pktcnode->GotNetmailBytes+=size;
   }

   /* Set zones if they are zero */

   if(mm->DestNode.Zone == 0)
      mm->DestNode.Zone = mm->PktDest.Zone;

   if(mm->OrigNode.Zone == 0)
      mm->OrigNode.Zone = mm->PktOrig.Zone;

   /* Add CR if last line doesn't end with CR */

   chunk=(struct TextChunk *)mm->TextChunks.First;

   if(chunk && chunk->Length!=0)
   {
      if(chunk->Data[chunk->Length-1] != 13 && chunk->Data[chunk->Length-1])
         mmAddBuf(&mm->TextChunks,"\x0d",1);
   }

	/* Filter */

   if(!Filter(mm))
      return(FALSE);

   if(mm->Flags & MMFLAG_KILL)
		return(TRUE);

   /* Check if it is to me */

   for(aka=(struct Aka *)config.AkaList.First;aka;aka=aka->Next)
      if(Compare4D(&mm->DestNode,&aka->Node)==0) break;

   if(aka)
   {
      /* AreaFix */

      if(!(mm->Flags & MMFLAG_AUTOGEN))
      {
         for(areafixname=(struct AreaFixName *)config.AreaFixList.First;areafixname;areafixname=areafixname->Next)
            if(stricmp(areafixname->Name,mm->To)==0) break;

         if(areafixname)
         {
            if(!AreaFix(mm))
               return(FALSE);

            if(!(config.cfg_Flags & CFG_IMPORTAREAFIX))
               return(TRUE);
         }
      }
   }

   /* Find correct area */

   for(tmparea=(struct Area *)config.AreaList.First;tmparea;tmparea=tmparea->Next)
      if(tmparea->AreaType == AREATYPE_NETMAIL)
      {
         if(Compare4D(&tmparea->Aka->Node,&mm->DestNode)==0)
            break;

         for(inode=(struct ImportNode *)tmparea->TossNodes.First;inode;inode=inode->Next)
            if(Compare4D(&inode->Node,&mm->DestNode)==0) break;

         if(inode)
            break;
      }
   

   /* If no area was found but it is to one of the akas, take first netmail area */
   /* Same if NOROUTE was specified in config */

   if(!tmparea)
   {
      for(aka=(struct Aka *)config.AkaList.First;aka;aka=aka->Next)
         if(Compare4D(&mm->DestNode,&aka->Node)==0) break;

      if(aka || (config.cfg_Flags & CFG_NOROUTE))
      {
         for(tmparea=(struct Area *)config.AreaList.First;tmparea;tmparea=tmparea->Next)
            if(tmparea->AreaType == AREATYPE_NETMAIL) break;
      }
   }

   if(tmparea)
   {
      /* Import netmail */

		if(mm->Flags & MMFLAG_TWIT)
			return(TRUE);

      if(config.cfg_Flags & CFG_STRIPRE)
         strcpy(mm->Subject,StripRe(mm->Subject));

      /* Import empty netmail? */

      istext=TRUE;

      if(!(config.cfg_Flags & CFG_IMPORTEMPTYNETMAIL))
      {
         istext=FALSE;

         for(chunk=(struct TextChunk *)mm->TextChunks.First;chunk && !istext;chunk=chunk->Next)
            for(c=0;c<chunk->Length && !istext;)
            {
               if(chunk->Data[c]!=1)
                  istext=TRUE;

               while(chunk->Data[c]!=13 && c<chunk->Length) c++;
               if(chunk->Data[c]==13) c++;
            }
      }

      if(istext)
      {
         if(!(mm->Flags & MMFLAG_TOSSED))
            LogWrite(3,TOSSINGINFO,"Importing message");

         tmparea->NewTexts++;

         if(tmparea->Messagebase)
			{
		      toss_import++;

   		   if(config.cfg_Flags & CFG_STRIPRE)
	      	   strcpy(mm->Subject,StripRe(mm->Subject));

				/* Remove LOCAL flag if set and set SENT flag */

				mm->Attr |= FLAG_SENT;
				mm->Attr &= ~(FLAG_LOCAL);

            if(!(*tmparea->Messagebase->importfunc)(mm,tmparea))
					return(FALSE);
			}
      }
      else
      {
         Print4D(&mm->OrigNode,buf);
         LogWrite(4,TOSSINGINFO,"Killed empty netmail from %s at %s",mm->From,buf);
      }

      if((mm->Attr & FLAG_RREQ) && (config.cfg_Flags & CFG_ANSWERRECEIPT))
      {
         if(!AnswerReceipt(mm))
            return(FALSE);
      }
   }
   else
   {
      /* Clear flags */

      mm->Attr&=(FLAG_PVT|FLAG_CRASH|FLAG_FILEATTACH|FLAG_HOLD|FLAG_RREQ|FLAG_IRRR|FLAG_AUDIT);

      if(mm->Flags & MMFLAG_TOSSED)
			mm->Attr&=~(FLAG_CRASH|FLAG_HOLD);

      mm->Type = PKTS_NORMAL;

      if(mm->Attr & FLAG_CRASH) mm->Type=PKTS_CRASH;
      if(mm->Attr & FLAG_HOLD)  mm->Type=PKTS_HOLD;

		/* File-attach? */

      if((mm->Attr & FLAG_FILEATTACH) && !(config.cfg_Flags & CFG_NODIRECTATTACH))
		{
			if(mm->Type == PKTS_NORMAL)
            mm->Type=PKTS_DIRECT;
		}

      /* Find route statement */

      for(tmproute=(struct Route *)config.RouteList.First;tmproute;tmproute=tmproute->Next)
         if(Compare4DPat(&tmproute->Pattern,&mm->DestNode)==0) break;

      if(!tmproute)
      {
         LogWrite(1,TOSSINGERR,"No routing configured for %lu:%lu/%lu.%lu - message lost",
            mm->DestNode.Zone,
            mm->DestNode.Net,
            mm->DestNode.Node,
            mm->DestNode.Point);

         toss_bad++;

         if(!WriteBad(mm,"No routing for destination node"))
            return(FALSE);

         return(TRUE);
      }

		/* Set destination */
		
		if(mm->Type == PKTS_NORMAL)
		{
			uchar buf1[50],buf2[50],buf3[50];

			Print4DPat(&tmproute->Pattern,buf1);
			Print4DPat(&tmproute->DestPat,buf2);
			Print4D(&tmproute->Aka->Node,buf3);
			LogWrite(6,DEBUG,"Uses this route statement: ROUTE \"%s\" \"%s\" \"%s\"",buf1,buf2,buf3);

         ExpandNodePat(&tmproute->DestPat,&mm->DestNode,&Dest4D);
		}
		else
		{
			Copy4D(&Dest4D,&mm->DestNode);
		}
		
	   /* Change */

	   oldtype=mm->Type;
   	mm->Type=ChangeType(&Dest4D,mm->Type);

	   if(mm->Type != oldtype)
   	{
      	LogWrite(4,TOSSINGINFO,"Changed priority for netmail to %lu:%lu/%lu.%lu from %s to %s",
				Dest4D.Zone,Dest4D.Net,Dest4D.Node,Dest4D.Point,
         	prinames[oldtype],prinames[mm->Type]);
		}

		if(Dest4D.Point != 0)
		{
			if(mm->Type == PKTS_DIRECT || mm->Type == PKTS_CRASH)
			{
				Dest4D.Point=0;

				LogWrite(4,TOSSINGINFO,"Cannot send %s to a point, sending to %lu:%lu/%lu.%lu instead",
					prinames[mm->Type],Dest4D.Zone,Dest4D.Net,Dest4D.Node,Dest4D.Point);
		   }
		}

      /* Check for loopmail */

      if(config.cfg_LoopMode != LOOP_IGNORE && !(mm->Flags & MMFLAG_EXPORTED))
      {
         if(IsLoopMail(mm))
         {
            LogWrite(1,TOSSINGERR,"Possible loopmail detected: Received from %lu:%lu/%lu.%lu, to %lu:%lu/%lu.%lu",
                  mm->PktOrig.Zone,
                  mm->PktOrig.Net,
                  mm->PktOrig.Node,
                  mm->PktOrig.Point,
                  mm->DestNode.Zone,
                  mm->DestNode.Net,
                  mm->DestNode.Node,
                  mm->DestNode.Point);

            if(config.cfg_LoopMode == LOOP_LOGBAD)
            {
               struct MemMessage *tmpmm;
               struct TextChunk *tmp;

               /* Make a copy of the message with only kludge lines */

               if(!(tmpmm=mmAlloc()))
                  return(FALSE);

               Copy4D(&tmpmm->PktOrig,&mm->PktOrig);
               Copy4D(&tmpmm->PktDest,&mm->PktDest);
               Copy4D(&tmpmm->OrigNode,&mm->OrigNode);
               Copy4D(&tmpmm->DestNode,&mm->DestNode);

               strcpy(tmpmm->Area,mm->Area);

               strcpy(tmpmm->To,mm->To);
               strcpy(tmpmm->From,mm->From);
               strcpy(tmpmm->Subject,mm->Subject);
               strcpy(tmpmm->DateTime,mm->DateTime);

               strcpy(tmpmm->MSGID,mm->MSGID);
               strcpy(tmpmm->REPLY,mm->REPLY);

               tmpmm->Attr=mm->Attr;
               tmpmm->Cost=mm->Cost;

               tmpmm->Type=mm->Type;
               tmpmm->Flags=mm->Flags;

               for(tmp=(struct TextChunk *)mm->TextChunks.First;tmp;tmp=tmp->Next)
               {
                  c=0;

                  while(c<tmp->Length)
                  {
                     for(d=c;d<tmp->Length && tmp->Data[d]!=13;d++);
                     if(tmp->Data[d]==13) d++;

                     if(tmp->Data[c]==1)
                     {
                        tmp->Data[c]='@';
                        mmAddBuf(&tmpmm->TextChunks,&tmp->Data[c],d-c);
                        tmp->Data[c]=1;
                     }
                     c=d;
                  }
               }

               toss_bad++;

               if(!WriteBad(tmpmm,"Possible loopmail"))
                  return(FALSE);

               mmFree(tmpmm);
            }
         }
      }

      /* Bounce if not in nodelist */

      if(config.cfg_NodelistType)
      {
         for(patternnode=(struct PatternNode *)config.BounceList.First;patternnode;patternnode=patternnode->Next)
            if(Compare4DPat(&patternnode->Pattern,&mm->DestNode)==0) break;

         if(patternnode)
         {
				struct Node4D node;

				Copy4D(&node,&mm->DestNode);
				node.Point=0;

            if(!(*config.cfg_NodelistType->nlCheckNode)(&node))
            {
               LogWrite(3,TOSSINGERR,"Bounced message from %u:%u/%u.%u to %u:%u/%u.%u -- not in nodelist",
                  mm->OrigNode.Zone,
                  mm->OrigNode.Net,
                  mm->OrigNode.Node,
                  mm->OrigNode.Point,
                  mm->DestNode.Zone,
                  mm->DestNode.Net,
                  mm->DestNode.Node,
                  mm->DestNode.Point);

               sprintf(buf,
                  "Warning! Your message has been bounced because the node %u:%u/%u doesn't exist in the nodelist.",
                     mm->DestNode.Zone,
                     mm->DestNode.Net,
                     mm->DestNode.Node);

               headeronly=FALSE;
               if(config.cfg_Flags & CFG_BOUNCEHEADERONLY) headeronly=TRUE;

               if(!Bounce(mm,buf,headeronly))
                  return(FALSE);

               return(TRUE);
            }
         }
      }

      /* Bounce if unconfigured point */

      if(config.cfg_Flags & CFG_BOUNCEPOINTS)
      {
         Copy4D(&n4d,&mm->DestNode);
         n4d.Point=0;

         for(aka=(struct Aka *)config.AkaList.First;aka;aka=aka->Next)
         {
            if(Compare4D(&aka->Node,&n4d)==0 && aka->Node.Point==0)
            {
               for(cnode=(struct ConfigNode *)config.CNodeList.First;cnode;cnode=cnode->Next)
                  if(Compare4D(&cnode->Node,&mm->DestNode)==0) break;

               if(!cnode)
               {
                  LogWrite(3,TOSSINGERR,"Bounced message from %u:%u/%u.%u to %u:%u/%u.%u -- unknown point",
                     mm->OrigNode.Zone,
                     mm->OrigNode.Net,
                     mm->OrigNode.Node,
                     mm->OrigNode.Point,
                     mm->DestNode.Zone,
                     mm->DestNode.Net,
                     mm->DestNode.Node,
                     mm->DestNode.Point);

                  sprintf(buf,"Warning! Your message has been bounced because the point %u:%u/%u.%u doesn't exist.",
                                 mm->DestNode.Zone,
                                 mm->DestNode.Net,
                                 mm->DestNode.Node,
                                 mm->DestNode.Point);

                  headeronly=FALSE;
                  if(config.cfg_Flags & CFG_BOUNCEHEADERONLY) headeronly=TRUE;

                  if(!Bounce(mm,buf,headeronly))
                     return(FALSE);

                  return(TRUE);
               }
            }
         }
      }

      /* Handle file-attach */

      if(mm->Attr & FLAG_FILEATTACH)
      {
         LogWrite(6,DEBUG,"Netmail is fileattach");

         if(!(mm->Flags & MMFLAG_EXPORTED))
         {
            for(patternnode=(struct PatternNode *)config.FileAttachList.First;patternnode;patternnode=patternnode->Next)
               if(Compare4DPat(&patternnode->Pattern,&mm->DestNode)==0) break;

            if(!patternnode)
            {
               LogWrite(3,TOSSINGERR,"Refused to route file-attach from %lu:%lu/%lu.%lu to %lu:%lu/%lu.%lu",mm->OrigNode.Zone,mm->OrigNode.Net,mm->OrigNode.Node,mm->OrigNode.Point,
                                                                                         mm->DestNode.Zone,mm->DestNode.Net,mm->DestNode.Node,mm->DestNode.Point);


               sprintf(buf,"Warning! Your message has been bounced because because routing of file-attaches to %u:%u/%u.%u is not allowed.",
                           mm->DestNode.Zone,mm->DestNode.Net,mm->DestNode.Node,mm->DestNode.Point);

               headeronly=FALSE;
               if(config.cfg_Flags & CFG_BOUNCEHEADERONLY) headeronly=TRUE;

               if(!Bounce(mm,buf,headeronly))
						return(FALSE);

					return(TRUE);
            }
         }

			strcpy(subjtemp,mm->Subject);
			mm->Subject[0]=0;

			for(c=0;subjtemp[c];c++)
         {
            if(subjtemp[c]==',')  subjtemp[c]=' ';
            if(subjtemp[c]=='\\') subjtemp[c]='/';
         }

			jbcpos=0;

			while(jbstrcpy(buf,subjtemp,80,&jbcpos))
			{
				if(mm->Subject[0] != 0) strcat(mm->Subject," ");
         	strcat(mm->Subject,GetFilePart(buf));

            LogWrite(4,TOSSINGINFO,"Routing file %s to %lu:%lu/%lu.%lu",GetFilePart(buf),Dest4D.Zone,Dest4D.Net,Dest4D.Node,Dest4D.Point);

            if((mm->Flags & MMFLAG_EXPORTED))
            {
					if(osExists(buf))
               {
						MakeFullPath(config.cfg_PacketDir,GetFilePart(buf),buf2,200);
                  copyfile(buf,buf2);

						if(nomem || ioerror)
							return(FALSE);

                  AddFlow(buf2,&Dest4D,mm->Type,FLOW_DELETE);
               }
               else
               {
                  AddFlow(buf,&Dest4D,mm->Type,FLOW_NONE);
               }
            }
            else
            {
					MakeFullPath(config.cfg_Inbound,GetFilePart(buf),buf2,200);
					MakeFullPath(config.cfg_PacketDir,GetFilePart(buf),buf3,200);

               if(movefile(buf2,buf3))
               {
                  AddFlow(buf3,&Dest4D,mm->Type,FLOW_DELETE);
               }
               else
               {
                  AddFlow(buf2,&Dest4D,mm->Type,FLOW_DELETE);
               }
            }
         }
      }

      time(&t);
      tp = localtime(&t);

      Print4D(&tmproute->Aka->Node,buf2);

      sprintf(buf,"\x01Via %s @%04u%02u%02u.%02u%02u%02u CrashMail II/" OS_PLATFORM_NAME " " VERSION "\x0d",
         buf2,
         tp->tm_year+1900,
         tp->tm_mon+1,
         tp->tm_mday,
         tp->tm_hour,
         tp->tm_min,
         tp->tm_sec);

      mmAddLine(mm,buf);

      if(mm->Type == PKTS_NORMAL)
      {
         LogWrite(5,TOSSINGINFO,"Routing message to %lu:%lu/%lu.%lu via %lu:%lu/%lu.%lu",
            mm->DestNode.Zone,
            mm->DestNode.Net,
            mm->DestNode.Node,
            mm->DestNode.Point,
            Dest4D.Zone,
            Dest4D.Net,
            Dest4D.Node,
            Dest4D.Point);
		}
      else
      {
         LogWrite(5,TOSSINGINFO,"Sending message directly to %lu:%lu/%lu.%lu",
            Dest4D.Zone,
            Dest4D.Net,
            Dest4D.Node,
            Dest4D.Point);
		}
		
      if(!WriteNetMail(mm,&Dest4D,tmproute->Aka))
         return(FALSE);

      if((mm->Attr & FLAG_AUDIT) && (config.cfg_Flags & CFG_ANSWERAUDIT) && (mm->Flags & MMFLAG_TOSSED))
      {
         if(!AnswerAudit(mm))
            return(FALSE);
      }

      toss_route++;
   }

   return(TRUE);
}


/******************************* end netmail **********************************/


/********************************** Rescan *******************************/

bool HandleRescan(struct MemMessage *mm)
{
   struct Area *temparea;

   rescan_total++;

   /* Find area */

   for(temparea=(struct Area *)config.AreaList.First;temparea;temparea=temparea->Next)
      if(stricmp(temparea->Tagname,mm->Area)==0) break;

   /* Add own node to seen-by to be on the safe side */

   if(temparea->Aka->Node.Point == 0)
   {
      if(!mmAddNodes2DList(&mm->SeenBy,temparea->Aka->Node.Net,temparea->Aka->Node.Node))
         return(FALSE);
   }

   /* Add destination node to seen-by to be on the safe side */

   if(RescanNode->Node.Point == 0)
   {
      if(!mmAddNodes2DList(&mm->SeenBy,RescanNode->Node.Net,RescanNode->Node.Node))
         return(FALSE);
   }

   /* Add own node to path */

   if(temparea->Aka->Node.Point == 0 || (config.cfg_Flags & CFG_PATH3D))
   {
      if(!AddNodePath(&mm->Path,&temparea->Aka->Node))
         return(FALSE);
   }

   if(!mmSortNodes2D(&mm->SeenBy))
      return(FALSE);

   if(!WriteEchoMail(mm,RescanNode,temparea->Aka))
      return(FALSE);

   return(TRUE);
}

