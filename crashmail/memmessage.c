#include "crashmail.h"

bool mmAddNodes2DList(struct jbList *list,uint16_t net,uint16_t node)
{
   /* Add a node to SEEN-BY list */

   struct Nodes2D *tmplist;
   uint16_t num;

   /* Check if it already exists */

   for(tmplist=(struct Nodes2D *)list->First;tmplist;tmplist=tmplist->Next)
   {
      for(num=0;num<tmplist->Nodes;num++)
         if(tmplist->Net[num]==net && tmplist->Node[num]==node) return(TRUE);
   }

   tmplist=(struct Nodes2D *)list->Last;

   if(tmplist && tmplist->Nodes == PKT_NUM2D)
      tmplist=NULL;

   if(!tmplist)
   {
      if(!(tmplist=(struct Nodes2D *)osAlloc(sizeof(struct Nodes2D))))
      {
         nomem=TRUE;
         return(FALSE);
      }

      jbAddNode(list,(struct jbNode *)tmplist);
      tmplist->Nodes=0;
      tmplist->Next=NULL;
   }

   tmplist->Net[tmplist->Nodes]=net;
   tmplist->Node[tmplist->Nodes]=node;
   tmplist->Nodes++;

   return(TRUE);
}

void mmRemNodes2DList(struct jbList *list,uint16_t net,uint16_t node)
{
   /* Rem a node from SEEN-BY list */

   struct Nodes2D *tmplist;
   uint16_t num;

   for(tmplist=(struct Nodes2D *)list->First;tmplist;tmplist=tmplist->Next)
      for(num=0;num<tmplist->Nodes;num++)
         if(tmplist->Net[num]==net && tmplist->Node[num]==num)
         {
            tmplist->Net[num]=0;
            tmplist->Node[num]=0;
         }
}

void mmRemNodes2DListPat(struct jbList *list,struct Node2DPat *pat)
{
   struct Nodes2D *tmplist;
   uint16_t num;

   for(tmplist=(struct Nodes2D *)list->First;tmplist;tmplist=tmplist->Next)
      for(num=0;num<tmplist->Nodes;num++)
      {
         if(Compare2DPat(pat,tmplist->Net[num],tmplist->Node[num])==0)
         {
            tmplist->Net[num]=0;
            tmplist->Node[num]=0;
         }
      }
}


bool mmAddPath(char *str,struct jbList *list)
{
   /* Add a node string to PATH list */

   struct Path *path;

   if(str[0]==0)
      return(TRUE);

   path=(struct Path *)list->Last;

   if(path && path->Paths == PKT_NUMPATH)
      path=NULL;

   if(!path)
   {
      if(!(path=(struct Path *)osAlloc(sizeof(struct Path))))
      {
         nomem=TRUE;
         return(FALSE);
      }

      jbAddNode(list,(struct jbNode *)path);
      path->Paths=0;
      path->Next=NULL;
   }

   mystrncpy(path->Path[path->Paths],str,100);
   striptrail(path->Path[path->Paths]);
   path->Paths++;

   return(TRUE);
}

bool mmAddBuf(struct jbList *chunklist,char *buf,uint32_t len)
{
   struct TextChunk *chunk,*oldchunk;
	uint32_t copylen,d;

   if(len == 0)
      return(TRUE);

   /* Find last chunk */

   chunk=(struct TextChunk *)chunklist->Last;

   /* Will it fit in this chunk?. If yes, copy and exit. */

   if(chunk && chunk->Length+len <= PKT_CHUNKLEN)
   {
      memcpy(&chunk->Data[chunk->Length],buf,(size_t)len);
      chunk->Length+=len;

      return(TRUE);
   }

   /* We will need a new chunk */

   while(len)
	{
      oldchunk=chunk;

      if(!(chunk=(struct TextChunk *)osAlloc(sizeof(struct TextChunk))))
      {
        	nomem=TRUE;
         return(FALSE);
      }

      chunk->Length=0;
    	jbAddNode(chunklist,(struct jbNode *)chunk);

      /* Copy over last line from old chunk if not complete */

      if(oldchunk && oldchunk->Length > 0 && oldchunk->Data[oldchunk->Length-1] != 13)
      {
         for(d=oldchunk->Length-1;d>0;d--)
            if(oldchunk->Data[d] == 13) break;

         if(d != 0)
         {
            memcpy(chunk->Data,&oldchunk->Data[d+1],oldchunk->Length-d-1);
            chunk->Length=oldchunk->Length-d-1;
            oldchunk->Length=d+1;
         }
      }

		copylen=len;
		if(copylen > PKT_CHUNKLEN-chunk->Length) copylen=PKT_CHUNKLEN-chunk->Length;

   	memcpy(&chunk->Data[chunk->Length],buf,(size_t)copylen);
	   chunk->Length+=copylen;

		buf=&buf[copylen];
		len-=copylen;
	}

   return(TRUE);
}

struct MemMessage *mmAlloc(void)
{
   struct MemMessage *mm;

   mm=osAllocCleared(sizeof(struct MemMessage));

   if(!mm)
   {
      osFree(mm);
      nomem=TRUE;
      return(NULL);
   }

   jbNewList(&mm->TextChunks);
   jbNewList(&mm->SeenBy);
   jbNewList(&mm->Path);

   return(mm);
}

void mmClear(struct MemMessage *mm)
{
   struct Node4D null4d = { 0,0,0,0 };

   Copy4D(&mm->OrigNode,&null4d);
   Copy4D(&mm->DestNode,&null4d);
   Copy4D(&mm->PktOrig,&null4d);
   Copy4D(&mm->PktDest,&null4d);

   Copy4D(&mm->Origin4D,&null4d);

   mm->Area[0]=0;

   mm->To[0]=0;
   mm->From[0]=0;
   mm->Subject[0]=0;
   mm->DateTime[0]=0;

   mm->MSGID[0]=0;
   mm->REPLY[0]=0;

   mm->Attr=0;
   mm->Cost=0;

   mm->Type=0;
   mm->Flags=0;

   jbFreeList(&mm->TextChunks);
   jbFreeList(&mm->SeenBy);
   jbFreeList(&mm->Path);
}

void mmFree(struct MemMessage *mm)
{
   jbFreeList(&mm->TextChunks);
   jbFreeList(&mm->SeenBy);
   jbFreeList(&mm->Path);

   osFree(mm);
}

struct TempSort
{
   uint16_t Net;
   uint16_t Node;
};

int CompareSort(const void *t1,const void *t2)
{
   if(((struct TempSort *)t1)->Net  > ((struct TempSort *)t2)->Net)
      return(1);

   if(((struct TempSort *)t1)->Net  < ((struct TempSort *)t2)->Net)
      return(-1);

   if(((struct TempSort *)t1)->Node > ((struct TempSort *)t2)->Node)
      return(1);

   if(((struct TempSort *)t1)->Node < ((struct TempSort *)t2)->Node)
      return(-1);

  return(0);
}

bool mmSortNodes2D(struct jbList *list)
{
   struct Nodes2D *tmp;
   struct TempSort *sorttemp;
   uint32_t nodes=0;
   uint32_t c,d;

   for(tmp=(struct Nodes2D *)list->First;tmp;tmp=tmp->Next)
      nodes+=tmp->Nodes;

   if(nodes==0)
      return(TRUE);

   if(!(sorttemp=(struct TempSort *)osAlloc(sizeof(struct TempSort)*nodes)))
   {
      nomem=TRUE;
      return(FALSE);
   }

   d=0;

   for(tmp=(struct Nodes2D *)list->First;tmp;tmp=tmp->Next)
      for(c=0;c<tmp->Nodes;c++)
      {
         sorttemp[d].Net=tmp->Net[c];
         sorttemp[d].Node=tmp->Node[c];
         d++;
      }

	qsort(sorttemp,(size_t)nodes,sizeof(struct TempSort),CompareSort);

   tmp=(struct Nodes2D *)list->First;
   tmp->Nodes=0;

   for(c=0;c<nodes;c++)
   {
      if(tmp->Nodes == PKT_NUM2D)
      {
         tmp=tmp->Next;
         tmp->Nodes=0;
      }
      tmp->Net[tmp->Nodes]=sorttemp[c].Net;
      tmp->Node[tmp->Nodes]=sorttemp[c].Node;
      tmp->Nodes++;
   }

   osFree(sorttemp);

   return(TRUE);
}

bool AddSeenby(char *str,struct jbList *list)
{
   /* Add a node string to SEEN-BY list */

   uint32_t c,d;
   char buf[60];
   uint16_t lastnet,num;

   c=0;
   lastnet=0;

   while(str[c]!=13 && str[c]!=0)
   {
      d=0;

      while(str[c]!=0 && str[c]!=13 && str[c]!=32 && d<59)
         buf[d++]=str[c++];

      buf[d]=0;
      while(str[c]==32) c++;

      if(buf[0])
      {
         num=0;
         d=0;

         while(buf[d]>='0' && buf[d]<='9')
         {
            num*=10;
            num+=buf[d]-'0';
            d++;
         }

         if(buf[d]=='/')
         {
            lastnet=num;
            num=atoi(&buf[d+1]);

            if(!mmAddNodes2DList(list,lastnet,num))
               return(FALSE);
         }
         else if(buf[d]==0)
         {
            if(!mmAddNodes2DList(list,lastnet,num))
               return(FALSE);
         }
      }
   }

   return(TRUE);
}

void ProcessKludge(struct MemMessage *mm,char *kludge)
{
   struct Node4D node;
   char buf[60];
   uint32_t c,d;

   if(strncmp(kludge,"\x01RESCANNED",10)==0)
   {
      mm->Flags |= MMFLAG_RESCANNED;
   }

   if(strncmp(kludge,"\x01MSGID:",7)==0)
   {
      for(d=0,c=8;d<79 && kludge[c]!=13 && kludge[c]!=0;c++,d++)
         mm->MSGID[d]=kludge[c];
         
      mm->MSGID[d]=0;
   }

   if(strncmp(kludge,"\x01REPLY:",7)==0)
   {
      for(d=0,c=8;d<79 && kludge[c]!=13 && kludge[c]!=0;c++,d++)
         mm->REPLY[d]=kludge[c];
         
      mm->REPLY[d]=0;
   }

   if(mm->Area[0]==0)
   {
      if(strncmp(kludge,"\x01" "FMPT",5)==0)
         mm->OrigNode.Point=atoi(&kludge[6]);

      if(strncmp(kludge,"\x01TOPT",5)==0)
          mm->DestNode.Point=atoi(&kludge[6]);

      if(strncmp(kludge,"\x01INTL",5)==0)
      {
         if(kludge[5]==':')
            c=7;

         else
            c=6;

         for(d=0;d<59 && kludge[c]!=32 && kludge[c]!=0;c++,d++)
            buf[d]=kludge[c];

         buf[d]=0;

         if(Parse4D(buf,&node))
         {
            mm->DestNode.Zone = node.Zone;
            mm->DestNode.Net  = node.Net;
            mm->DestNode.Node = node.Node;
         }

         if(kludge[c]==32) c++;

         for(d=0;d<59 && kludge[c]!=32 && kludge[c]!=0 && kludge[c]!=13;c++,d++)
            buf[d]=kludge[c];

         buf[d]=0;

         if(Parse4D(buf,&node))
         {
            mm->OrigNode.Zone = node.Zone;
            mm->OrigNode.Net  = node.Net;
            mm->OrigNode.Node = node.Node;
         }
      }
   }
}


bool mmAddLine(struct MemMessage *mm,char *buf)
{
   if(mm->Area[0] && strncmp(buf,"SEEN-BY:",8)==0)
      return AddSeenby(&buf[9],&mm->SeenBy);

   else if(mm->Area[0] && strncmp(buf,"\x01PATH:",6)==0)
      return mmAddPath(&buf[7],&mm->Path);

 	else if(mm->Area[0] && strncmp(buf," * Origin: ",11)==0)
	{
		struct Node4D n4d;

   	if(ExtractAddress(buf,&n4d))
		{
      	if(n4d.Zone == 0) n4d.Zone=mm->PktOrig.Zone;
			Copy4D(&mm->Origin4D,&n4d);
      }
	}

   else if(buf[0] == 1)
      ProcessKludge(mm,buf);

   return mmAddBuf(&mm->TextChunks,buf,(uint32_t)strlen(buf));
}

char *mmMakeSeenByBuf(struct jbList *list)
{
   char *text;
   uint32_t seenbys,size;
   struct Nodes2D *nodes;
   uint32_t c;
   uint16_t lastnet;
   char buf[100],buf2[50],buf3[20];

   /* Count seenbys */

   seenbys=0;

   for(nodes=(struct Nodes2D *)list->First;nodes;nodes=nodes->Next)
      seenbys+=nodes->Nodes;

   /* We allocate generously. Maximum length per seenby: <space>12345/12345: 12 characters
      79 characters - "SEEN-BY:" = 71 characters which makes 71/12 seenbys/line = 5

      Fortunately even with this generous calculation will not result in huge memory
      areas, 500 seenbys (which is a lot) would need 8000 bytes...                        */

   size=(seenbys/5+1)*80;

   /* Allocate our memory block */

   if(!(text=osAlloc(size)))
      return(NULL);

   text[0]=0;

   strcpy(buf,"SEEN-BY:");
   lastnet=0;

   for(nodes=(struct Nodes2D *)list->First;nodes;nodes=nodes->Next)
   {
      for(c=0;c<nodes->Nodes;c++)
      {
         if(nodes->Net[c]!=0 || nodes->Node[c]!=0)
         {
            strcpy(buf2," ");

            if(nodes->Net[c]!=lastnet)
            {
               sprintf(buf3,"%u/",nodes->Net[c]);
               strcat(buf2,buf3);
            }

            sprintf(buf3,"%u",nodes->Node[c]);
            strcat(buf2,buf3);

            lastnet=nodes->Net[c];

            if(strlen(buf)+strlen(buf2) > 77)
            {
               strcat(text,buf);
               strcat(text,"\x0d");

               strcpy(buf,"SEEN-BY:");
               sprintf(buf2," %u/%u",nodes->Net[c],nodes->Node[c]);
               lastnet=nodes->Net[c];

               strcat(buf,buf2);
            }
            else
            {
               strcat(buf,buf2);
            }
         }
      }
   }

   if(strlen(buf)>8)
   {
      strcat(text,buf);
      strcat(text,"\x0d");
   }

   return(text);
}
