#include "crashmail.h"

bool HandleEchomail(struct MemMessage *mm);
bool HandleNetmail(struct MemMessage *mm);
bool HandleRescan(struct MemMessage *mm);

bool HandleMessage(struct MemMessage *mm)
{
   LogWrite(6,DEBUG,"Is in HandleMessage()");

   if(istossing)
      toss_total++;

   if(mm->Area[0]==0)
      return HandleNetmail(mm);

   else
      return HandleEchomail(mm);
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
      if(temparea->Flags & AREA_BAD) break;

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

   if(istossing && !mm->no_security)
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

   if(config.cfg_DupeMode!=DUPE_IGNORE && istossing && !(temparea->Flags & AREA_IGNOREDUPES))
   {
      if(CheckDupe(mm))
      {
         LogWrite(4,TOSSINGERR,"Duplicate message in %s",mm->Area);

         toss_dupes++;
         temparea->NewDupes++;

         if(istossing && tempcnode)
            tempcnode->Dupes++;

         if(config.cfg_DupeMode == DUPE_BAD)
         {
            if(!WriteBad(mm,"Duplicate message"))
               return(FALSE);
         }               

         return(TRUE);
      }
   }

   temparea->NewTexts++;

   if(!mmSortNodes2D(&mm->SeenBy))
      return(FALSE);

   /* Write to all nodes */

   if(!mm->Rescanned)
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

   if(istossing && temparea->Messagebase)
   {
      toss_import++;

      if(config.cfg_Flags & CFG_STRIPRE)
         strcpy(mm->Subject,StripRe(mm->Subject));

      (*temparea->Messagebase->importfunc)(mm,temparea);
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

/* Expand destpat */

bool iswildcard(uchar *str)
{
   int c;

   for(c=0;c<strlen(str);c++)
      if(str[c]=='*' || str[c]=='?') return(TRUE);

   return(FALSE);
}

void ExpandNodePat(struct Node4DPat *temproute,struct Node4D *dest,struct Node4D *sendto)
{
   long region,hub;
   
   region=0;
   hub=0; 
   
   if(config.cfg_NodelistType)
   {
      if(temproute->Type == PAT_REGION) region=(*config.cfg_NodelistType->nlGetRegion)(dest);
      if(temproute->Type == PAT_HUB) hub=(*config.cfg_NodelistType->nlGetHub)(dest);
     
      if(region == -1) region=0;
      if(hub == -1) hub=0;
   }
   
   if(region == 0) region=dest->Net;
   
   switch(temproute->Type)
   {
      case PAT_PATTERN:
         sendto->Zone  = iswildcard(temproute->Zone)  ? dest->Zone  : atoi(temproute->Zone);
         sendto->Net   = iswildcard(temproute->Net)   ? dest->Net   : atoi(temproute->Net);
         sendto->Node  = iswildcard(temproute->Node)  ? dest->Node  : atoi(temproute->Node);
         sendto->Point = iswildcard(temproute->Point) ? dest->Point : atoi(temproute->Point);
         break;

      case PAT_ZONE:
         sendto->Zone = dest->Zone;
         sendto->Net = dest->Zone;
         sendto->Node = 0;
         sendto->Point = 0;
         break;

      case PAT_REGION:
         sendto->Zone = dest->Zone;
         sendto->Net = region;
         sendto->Node = 0;
         sendto->Point = 0;
         break;

      case PAT_NET:
         sendto->Zone = dest->Zone;
         sendto->Net = dest->Net;
         sendto->Node = 0;
         sendto->Point = 0;
         break;

      case PAT_HUB:
         sendto->Zone = dest->Zone;
         sendto->Net = dest->Net;
         sendto->Node = hub;
         sendto->Point = 0;
         break;

      case PAT_NODE:
         sendto->Zone = dest->Zone;
         sendto->Net = dest->Net;
         sendto->Node = dest->Node;
         sendto->Point = 0;
         break;
   }
}

/* Bouncing and receipts */

bool Bounce(struct MemMessage *mm,uchar *str)
{
   uchar buf[100];
   ulong c;
   struct Route *tmproute;
   struct MemMessage *tmpmm;
   struct TextChunk *chunk;

   for(tmproute=(struct Route *)config.RouteList.First;tmproute;tmproute=tmproute->Next)
      if(Compare4DPat(&tmproute->Pattern,&mm->OrigNode)==0) break;

   if(!tmproute)
   {
      Print4D(&mm->OrigNode,buf);
      LogWrite(1,TOSSINGERR,"Can't bounce message, no routing for %s",buf);
      return(TRUE);
   }

   if(!(tmpmm=mmAlloc()))
      return(FALSE);

   Copy4D(&tmpmm->DestNode,&mm->OrigNode);
   Copy4D(&tmpmm->OrigNode,&tmproute->Aka->Node);

   strcpy(tmpmm->To,mm->From);
   strcpy(tmpmm->From,config.cfg_Sysop);
   strcpy(tmpmm->Subject,"Bounced message");

   tmpmm->Attr=FLAG_PVT;

   MakeFidoDate(time(NULL),tmpmm->DateTime);

   tmpmm->isbounce=TRUE;

   MakeNetmailKludges(tmpmm);

   if(config.cfg_Flags & CFG_ADDTID)
      AddTID(tmpmm);

   mmAddLine(tmpmm,str);

   if(config.cfg_Flags & CFG_BOUNCEHEADERONLY)
      mmAddLine(tmpmm,"This is the header of the message that was bounced:\x0d\x0d");

   else
      mmAddLine(tmpmm,"This is the message that was bounced:\x0d\x0d");

   sprintf(buf,"From: %-40.40s (%u:%u/%u.%u)\x0d",mm->From,
      mm->OrigNode.Zone,
      mm->OrigNode.Net,
      mm->OrigNode.Node,
      mm->OrigNode.Point);

   mmAddLine(tmpmm,buf);

   sprintf(buf,"  To: %-40.40s (%u:%u/%u.%u)\x0d",
      mm->To,mm->DestNode.Zone,
      mm->DestNode.Net,
      mm->DestNode.Node,
      mm->DestNode.Point);

   mmAddLine(tmpmm,buf);

   sprintf(buf,"Subj: %s\x0d",mm->Subject);

   mmAddLine(tmpmm,buf);

   sprintf(buf,"Date: %s\x0d\x0d",mm->DateTime);

   mmAddLine(tmpmm,buf);

   if(!(config.cfg_Flags & CFG_BOUNCEHEADERONLY))
   {
      for(chunk=(struct TextChunk *)mm->TextChunks.First;chunk;chunk=chunk->Next)
      {
         for(c=0;c<chunk->Length;c++)
            if(chunk->Data[c]==1) chunk->Data[c]='@';

         mmAddBuf(&tmpmm->TextChunks,chunk->Data,chunk->Length);
      }
   }

   HandleNetmail(tmpmm);

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

   tmpmm->isbounce=TRUE;

   MakeNetmailKludges(tmpmm);

   if(config.cfg_Flags & CFG_ADDTID)
      AddTID(tmpmm);

    sprintf(buf,"Your message to %s dated %s with the subject \"%s\" has reached its final "
               "destination. This message doesn't mean that the message has been read, it "
               "just tells you that it has arrived at this system.\x0d\x0d",
               mm->To,mm->DateTime,mm->Subject);

   mmAddLine(tmpmm,buf);
   HandleNetmail(tmpmm);
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

   tmpmm->isbounce=TRUE;

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
   
   HandleNetmail(tmpmm);

   mmFree(tmpmm);
   
   return(TRUE);
}

/* Changes FMPT, TOPT and INTL to new addresses and adds a ^aRemapped line */

bool Remap(struct MemMessage *mm,struct Node4D *newdest,uchar *newto)
{
   struct Route *tmproute;
   struct jbList oldlist;
   struct TextChunk *tmp;
   uchar buf[100];
   ulong c,d;
	bool skip;
	
	oldlist.First=mm->TextChunks.First;
	oldlist.Last=mm->TextChunks.Last;
	
	jbNewList(&mm->TextChunks);

	Copy4D(&mm->DestNode,newdest);

   if(strcmp(newto,"*")!=0)
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

   for(tmproute=(struct Route *)config.RouteList.First;tmproute;tmproute=tmproute->Next)
      if(Compare4DPat(&tmproute->Pattern,&mm->DestNode)==0) break;

   if(tmproute)
   {
      sprintf(buf,"\x01Remapped to %u:%u/%u.%u at %u:%u/%u.%u\x0d",newdest->Zone,
                                                                   newdest->Net,
                                                                   newdest->Node,
                                                                   newdest->Point,
                                                                   tmproute->Aka->Node.Zone,
                                                                   tmproute->Aka->Node.Net,
                                                                   tmproute->Aka->Node.Node,
                                                                   tmproute->Aka->Node.Point);

		mmAddLine(mm,buf);
   }

   jbFreeList(&oldlist);

	if(nomem)
		return(FALSE);

	return(TRUE);
}

/* WriteRFC and WriteMSG */

bool WriteRFC(struct MemMessage *mm,uchar *name)
{
   osFile fh;
   uchar *domain;
   struct Aka *aka;
   struct TextChunk *tmp;
   ulong c,d,lastspace;
   uchar buffer[100];

   if(!(fh=osOpen(name,MODE_NEWFILE)))
   {
      LogWrite(1,SYSTEMERR,"Unable to write RFC-message to %s",name);
      return(FALSE);
   }

   /* Write header */

   for(aka=(struct Aka *)config.AkaList.First;aka;aka=aka->Next)
      if(Compare4D(&mm->DestNode,&aka->Node)==0) break;

   domain="FidoNet"; 
 
   if(aka && aka->Domain[0]!=0)
      domain=aka->Domain;

   osFPrintf(fh,"From: %lu:%lu/%lu.%lu@%s (%s)\n",mm->OrigNode.Zone,
                                                  mm->OrigNode.Net,
                                                  mm->OrigNode.Node,
                                                  mm->OrigNode.Point,
                                                  domain,
                                                  mm->From);

   osFPrintf(fh,"To: %lu:%lu/%lu.%lu@%s (%s)\n",mm->DestNode.Zone,
                                                mm->DestNode.Net,
                                                mm->DestNode.Node,
                                                mm->DestNode.Point,
                                                domain,
                                                mm->To);

   osFPrintf(fh,"Subject: %s\n",mm->Subject);

   osFPrintf(fh,"Date: %s\n",mm->DateTime);

   if(mm->MSGID[0]!=0)
      osFPrintf(fh,"Message-ID: <%s>\n",mm->MSGID);

   if(mm->REPLY[0]!=0)
      osFPrintf(fh,"References: <%s>\n",mm->REPLY);

   /* Write kludges */

   for(tmp=(struct TextChunk *)mm->TextChunks.First;tmp;tmp=tmp->Next)
   {
      c=0;

      while(c<tmp->Length)
      {
         for(d=c;d<tmp->Length && tmp->Data[d]!=13 && tmp->Data[d]!=10;d++);
         if(tmp->Data[d]==13 || tmp->Data[d]==10) d++;

         if(tmp->Data[c]==1)
         {
            osPuts(fh,"X-Fido-");
            if(d-c-2!=0) osWrite(fh,&tmp->Data[c+1],d-c-2);
            osPuts(fh,"\n");
         }
         c=d;
      }
   }

   /* Write end-of-header */

   osPuts(fh,"\n");

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
            osPuts(fh,buffer);
            osPutChar(fh,'\n');
         }
      }
   }

   osClose(fh);

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
      printf("Failed to write to %s\n",file);
      return(FALSE);
   }

   /* Write header */

   osWrite(fh,&Msg,sizeof(struct StoredMsg));

   /* Write text */

   for(chunk=(struct TextChunk *)mm->TextChunks.First;chunk;chunk=chunk->Next)
      osWrite(fh,chunk->Data,chunk->Length);

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
         osWrite(fh,sbbuf,(ulong)strlen(sbbuf));

      osFree(sbbuf);
   }

   /* Write path */

   for(path=(struct Path *)mm->Path.First;path;path=path->Next)
      for(c=0;c<path->Paths;c++)
         if(path->Path[c][0]!=0)
         {
            osWrite(fh,"\x01PATH: ",7);
            osWrite(fh,path->Path[c],(ulong)strlen(path->Path[c]));
            osWrite(fh,"\x0d",1);
         }

   osPutChar(fh,0);

   osClose(fh);

   if(diskfull)
      return(FALSE);

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
   struct Remap      *remap;
   struct RemapNode  *remapnode;
	struct Node4D     n4d,Dest4D;
   struct PatternNode *patternnode;
   struct AreaFixName *areafixname;
   struct Robot *robot;
   struct TextChunk *tmpchunk,*chunk;
   bool istext;
   uchar buf[400],buf2[200],buf3[200],subjtemp[80];
   ulong c,d,arcres;
   bool found;
   time_t t;
   struct tm *tp;
   ulong size;
	uchar oldtype;
	
   /* Find orignode */

   for(pktcnode=(struct ConfigNode *)config.CNodeList.First;pktcnode;pktcnode=pktcnode->Next)
      if(Compare4D(&mm->PktOrig,&pktcnode->Node)==0) break;

   /* Calculate size */

   size=0;

   for(tmpchunk=(struct TextChunk *)mm->TextChunks.First;tmpchunk;tmpchunk=tmpchunk->Next)
      size+=tmpchunk->Length;

   /* Statistics */

   if(istossing && !isrescanning && pktcnode)
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

   /* Remap node */

   for(remapnode=(struct RemapNode *)config.RemapNodeList.First;remapnode;remapnode=remapnode->Next)
      if(Compare4DPat(&remapnode->NodePat,&mm->DestNode)==0) break;

   if(remapnode)
   {
      struct Node4D remap4d,t4d;

      Copy4D(&t4d,&mm->DestNode);
      ExpandNodePat(&remapnode->DestPat,&mm->DestNode,&remap4d);

      if(!Remap(mm,&remap4d,mm->To))
         return(FALSE);

      LogWrite(4,TOSSINGINFO,"Remapped message to %lu:%lu/%lu.%lu to %lu:%lu/%lu.%lu",
         t4d.Zone,
         t4d.Net,
         t4d.Node,
         t4d.Point,
         remap4d.Zone,
         remap4d.Net,
         remap4d.Node,
         remap4d.Point);
   }

   /* Check if it is to me */

   for(aka=(struct Aka *)config.AkaList.First;aka;aka=aka->Next)
      if(Compare4D(&mm->DestNode,&aka->Node)==0) break;

   if(aka)
   {
      /* Remap name */

      for(remap=(struct Remap *)config.RemapList.First;remap;remap=remap->Next)
         if(osMatchPattern(remap->Pattern,mm->To)) break;

      if(remap)
      {
			strcpy(buf,mm->To);
         ExpandNodePat(&remap->DestPat,&mm->DestNode,&n4d);

         if(!Remap(mm,&n4d,remap->NewTo))
            return(FALSE);

         LogWrite(4,TOSSINGINFO,"Remapped message to %s to %s at %lu:%lu/%lu.%lu",
																						buf,
                                                                  mm->To,
                                                                  n4d.Zone,
                                                                  n4d.Net,
                                                                  n4d.Node,
                                                                  n4d.Point);
      }

      /* Robotnames */

      for(robot=(struct Robot *)config.RobotList.First;robot;robot=robot->Next)
         if(osMatchPattern(robot->Pattern,mm->To)) break;

      if(robot)
      {
         bool msg,rfc;
         uchar msgbuf[L_tmpnam],rfcbuf[L_tmpnam];
         uchar origbuf[30],destbuf[30];

         msg=FALSE;
         rfc=FALSE;

         msgbuf[0]=0;
         rfcbuf[0]=0;

         if(strstr(robot->Command,"%m")) msg=TRUE;
         if(strstr(robot->Command,"%m")) rfc=TRUE;

         if(rfc) tmpnam(rfcbuf);
         if(msg) tmpnam(msgbuf);

         Print4D(&mm->OrigNode,origbuf);
         Print4D(&mm->DestNode,destbuf);

         ExpandRobot(robot->Command,buf,400,
            rfcbuf,
            msgbuf,
            mm->Subject,
            mm->DateTime,
            mm->From,
            mm->To,
            origbuf,
            destbuf);

         if(rfc) WriteRFC(mm,rfcbuf);
         if(msg) WriteMSG(mm,msgbuf); 

         LogWrite(4,SYSTEMINFO,"Executing external command \"%s\"",buf);

         arcres=system(buf);

         if(rfc) remove(rfcbuf);
         if(msg) remove(msgbuf);

         if(arcres == 0)
         {
            /* Command ok*/
            return(TRUE);
         }
         
         if(arcres >= 20)
         {
            LogWrite(1,SYSTEMERR,"External command \"%s\" failed with error %lu, exiting...",buf,arcres);
            return(FALSE);
         }
      }

      /* AreaFix */

      if(!mm->isbounce)
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
      if(tmparea->Flags & AREA_NETMAIL)
      {
         if(Compare4D(&tmparea->Aka->Node,&mm->DestNode)==0)
            break;

         for(inode=(struct ImportNode *)tmparea->TossNodes.First;inode && !found;inode=inode->Next)
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

      if(aka || (istossing && (config.cfg_Flags & CFG_NOROUTE)))
      {
         for(tmparea=(struct Area *)config.AreaList.First;tmparea;tmparea=tmparea->Next)
            if(tmparea->Flags & AREA_NETMAIL) break;
      }
   }

   if(tmparea)
   {
      /* Import netmail */

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
         if(isscanning)
            LogWrite(3,TOSSINGINFO,"Importing message");

         tmparea->NewTexts++;

         if(tmparea->Messagebase)
            (*tmparea->Messagebase->importfunc)(mm,tmparea);
      }
      else
      {
         Print4D(&mm->OrigNode,buf);
         LogWrite(4,TOSSINGINFO,"Killed empty netmail from %s at %ld",mm->From,buf);
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

      if(istossing) 
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

      if(config.cfg_LoopMode != LOOP_IGNORE && !isscanning)
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
               tmpmm->Rescanned=mm->Rescanned;
               tmpmm->no_security=mm->no_security;
               tmpmm->isbounce=mm->isbounce;

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

      if(config.cfg_NodelistType && !mm->isbounce)
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
                  "Warning! Your message has been bounced because the node "
                  "%u:%u/%u doesn't exist in the nodelist.\x0d"
                  "\x0d",
                     mm->DestNode.Zone,
                     mm->DestNode.Net,
                     mm->DestNode.Node);

               if(!Bounce(mm,buf))
                  return(FALSE);

               return(TRUE);
            }
         }
      }

      /* Bounce if unconfigured point */

      if((config.cfg_Flags & CFG_BOUNCEPOINTS) && !mm->isbounce)
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

                  sprintf(buf,"Warning! Your message has been bounced because the point "
                              "%u:%u/%u.%u doesn't exist.\x0d"
                              "\x0d",
                                 mm->DestNode.Zone,
                                 mm->DestNode.Net,
                                 mm->DestNode.Node,
                                 mm->DestNode.Point);

                  if(!Bounce(mm,buf))
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

         if(!isscanning)
         {
            for(patternnode=(struct PatternNode *)config.FileAttachList.First;patternnode;patternnode=patternnode->Next)
               if(Compare4DPat(&patternnode->Pattern,&mm->DestNode)==0) break;

            if(!patternnode)
            {
               LogWrite(3,TOSSINGERR,"Refused to route file-attach from %lu:%lu/%lu.%lu to %lu:%lu/%lu.%lu",mm->OrigNode.Zone,mm->OrigNode.Net,mm->OrigNode.Node,mm->OrigNode.Point,
                                                                                         mm->DestNode.Zone,mm->DestNode.Net,mm->DestNode.Node,mm->DestNode.Point);


               sprintf(buf,"Warning! Your message has been bounced because because routing "
                           "of file-attaches to %u:%u/%u.%u is not allowed.\x0d"
                           "\x0d",mm->DestNode.Zone,mm->DestNode.Net,mm->DestNode.Node,mm->DestNode.Point);

               if(!Bounce(mm,buf))
						return(FALSE);
						
					return(TRUE);
            }
         }

         c=0;
         subjtemp[0]=0;

         while(mm->Subject[c]!=0)
         {
            d=0;
            while(mm->Subject[c]!=0 && mm->Subject[c]!=32 && mm->Subject[c]!=',' && d<80)
               buf[d++]=mm->Subject[c++];

            buf[d]=0;

            while(mm->Subject[c]==32 || mm->Subject[c]==',') c++;

            if(buf[0]!=0)
            {
               strcat(subjtemp,GetFilePart(buf));

               LogWrite(4,TOSSINGINFO,"Routing file %s to %lu:%lu/%lu.%lu",GetFilePart(buf),Dest4D.Zone,Dest4D.Net,Dest4D.Node,Dest4D.Point);

               if(isscanning)
               {
						if(osExists(buf))
                  {
							MakeFullPath(config.cfg_PacketDir,GetFilePart(buf),buf2,200);
                     CopyFile(buf,buf2);
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

                  if(MoveFile(buf2,buf3))
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

         strcpy(mm->Subject,subjtemp);
      }

      time(&t);
      tp = localtime(&t);

      Print4D(&tmproute->Aka->Node,buf2);

      sprintf(buf,"\x01Via %s @%04u%02u%02u.%02u%02u%02u CrashMailII/" OS_PLATFORM_NAME " " VERSION "\x0d",
         buf2,
         tp->tm_year+1900,
         tp->tm_mon,
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

      if((mm->Attr & FLAG_AUDIT) && (config.cfg_Flags & CFG_ANSWERAUDIT) && istossing)
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

