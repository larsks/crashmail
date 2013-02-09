#include "crashmail.h"

bool rawParse4DPat(char *buf, struct Node4DPat *node)
{
   uint32_t c=0,tempc=0;
   char temp[10];
   bool GotZone=FALSE,GotNet=FALSE,GotNode=FALSE;

   strcpy(node->Zone,"*");
   strcpy(node->Net,"*");
   strcpy(node->Node,"*");
   strcpy(node->Point,"*");

   if(strcmp(buf,"*")==0)
      return(TRUE);

	for(c=0;c<strlen(buf);c++)
	{
		if(buf[c]==':')
		{
         if(GotZone || GotNet || GotNode) return(FALSE);
         strcpy(node->Zone,temp);
         GotZone=TRUE;
			tempc=0;
	   }
		else if(buf[c]=='/')
		{
         if(GotNet || GotNode) return(FALSE);
         strcpy(node->Net,temp);
         GotNet=TRUE;
			tempc=0;
		}
		else if(buf[c]=='.')
		{
         if(GotNode) return(FALSE);
         strcpy(node->Node,temp);
         node->Point[0]=0;
         GotNode=TRUE;
			tempc=0;
		}
		else if((buf[c]>='0' && buf[c]<='9') || buf[c]=='*' || buf[c]=='?')
		{
         if(tempc<9)
         {
            temp[tempc++]=buf[c];
            temp[tempc]=0;
         }
		}
		else return(FALSE);
	}

   if(GotZone && !GotNet)
   {
      strcpy(node->Net,temp);
   }
   else if(GotNode)
   {
      strcpy(node->Point,temp);
   }
   else
   {
      strcpy(node->Node,temp);
      strcpy(node->Point,"0");
   }

   return(TRUE);
}

bool Parse4DPat(char *buf, struct Node4DPat *node)
{
   bool res;

   node->Zone[0]=0;
   node->Net[0]=0;
   node->Node[0]=0;
   node->Point[0]=0;

   if(strnicmp(buf,"ZONE ",5)==0)
   {
      node->Type=PAT_ZONE;
      res=rawParse4DPat(&buf[5],node);
      strcpy(node->Zone,node->Node);
      node->Net[0]=0;
      node->Node[0]=0;
      node->Point[0]=0;
   }
   else if(strnicmp(buf,"REGION ",7)==0)
   {
      node->Type=PAT_REGION;
      res=rawParse4DPat(&buf[7],node);
      node->Node[0]=0;
      node->Point[0]=0;
   }
   else if(strnicmp(buf,"NET ",4)==0)
   {
      node->Type=PAT_NET;
      res=rawParse4DPat(&buf[4],node);
      node->Node[0]=0;
      node->Point[0]=0;
   }
   else if(strnicmp(buf,"HUB ",4)==0)
   {
      node->Type=PAT_HUB;
      res=rawParse4DPat(&buf[4],node);
      node->Point[0]=0;
   }
   else if(strnicmp(buf,"NODE ",5)==0)
   {
      node->Type=PAT_NODE;
      res=rawParse4DPat(&buf[5],node);
      node->Point[0]=0;
   }
   else
   {
      node->Type=PAT_PATTERN;
      res=rawParse4DPat(buf,node);
   }

   return(res);
}

bool Parse4DDestPat(char *buf, struct Node4DPat *node)
{
   bool res;

   node->Zone[0]=0;
   node->Net[0]=0;
   node->Node[0]=0;
   node->Point[0]=0;

   res=TRUE;

   if(stricmp(buf,"ZONE")==0)
   {
      node->Type=PAT_ZONE;
   }
   else if(stricmp(buf,"REGION")==0)
   {
      node->Type=PAT_REGION;
   }
   else if(stricmp(buf,"NET")==0)
   {
      node->Type=PAT_NET;
   }
   else if(stricmp(buf,"HUB")==0)
   {
      node->Type=PAT_HUB;
   }
   else if(stricmp(buf,"NODE")==0)
   {
      node->Type=PAT_NODE;
   }
   else
   {
      node->Type=PAT_PATTERN;
      res=rawParse4DPat(buf,node);
   }

   return(res);
}

int NodeCompare(char *pat,uint16_t num)
{
   char buf[10];
   uint8_t c;
   sprintf(buf,"%u",num);

   if(pat[0]==0) return(0);

   for(c=0;c<strlen(pat);c++)
   {
      if(pat[c]=='*') return(0);
      if(pat[c]!=buf[c] && pat[c]!='?') return(1);
      if(buf[c]==0) return(1);
   }

   if(buf[c]!=0)
      return(1);

   return(0);
}

int Compare4DPat(struct Node4DPat *nodepat,struct Node4D *node)
{
   int num;
	struct Node4D n4d;
	
   switch(nodepat->Type)
   {
      case PAT_PATTERN:
         if(node->Zone!=0)
            if(NodeCompare(nodepat->Zone, node->Zone )!=0) return(1);

         if(NodeCompare(nodepat->Net,  node->Net  )!=0) return(1);
         if(NodeCompare(nodepat->Node, node->Node )!=0) return(1);
         if(NodeCompare(nodepat->Point,node->Point)!=0) return(1);
         return(0);

      case PAT_ZONE:
         if(NodeCompare(nodepat->Zone,node->Zone)!=0) return(1);
         return(0);

      case PAT_REGION:
         if(!nodelistopen)
            return(1);

			Copy4D(&n4d,node);
			n4d.Point=0;
         num=(*config.cfg_NodelistType->nlGetRegion)(&n4d);

         if(num == -1)
            return(1);

         if(node->Zone!=0)
            if(NodeCompare(nodepat->Zone,node->Zone)!=0) return(1);

         if(NodeCompare(nodepat->Net,num)!=0) return(1);
         return(0);

      case PAT_NET:
         if(node->Zone!=0)
            if(NodeCompare(nodepat->Zone,node->Zone)!=0) return(1);

         if(NodeCompare(nodepat->Net,node->Net)!=0) return(1);
         return(0);

      case PAT_HUB:
         if(!nodelistopen)
            return(1);

			Copy4D(&n4d,node);
			n4d.Point=0;
         num=(*config.cfg_NodelistType->nlGetHub)(&n4d);

         if(num == -1)
            return(1);

         if(node->Zone!=0)
            if(NodeCompare(nodepat->Zone,node->Zone)!=0) return(1);

         if(NodeCompare(nodepat->Net,node->Net)!=0) return(1);
         if(NodeCompare(nodepat->Node,num)!=0) return(1);
         return(0);

      case PAT_NODE:
         if(node->Zone!=0)
            if(NodeCompare(nodepat->Zone, node->Zone )!=0) return(1);

         if(NodeCompare(nodepat->Net,  node->Net  )!=0) return(1);
         if(NodeCompare(nodepat->Node, node->Node )!=0) return(1);
         return(0);
   }

	return(1);
}

void Print4DPat(struct Node4DPat *pat,char *dest)
{
   switch(pat->Type)
   {
      case PAT_PATTERN:
         sprintf(dest,"%s:%s/%s.%s",pat->Zone,pat->Net,pat->Node,pat->Point);
         break;
      case PAT_ZONE:
         sprintf(dest,"ZONE %s",pat->Zone);
         break;
      case PAT_REGION:
         sprintf(dest,"REGION %s:%s",pat->Zone,pat->Net);
         break;
      case PAT_NET:
         sprintf(dest,"NET %s:%s",pat->Zone,pat->Net);
         break;
      case PAT_HUB:
         sprintf(dest,"HUB %s:%s/%s",pat->Zone,pat->Net,pat->Node);
         break;
      case PAT_NODE:
         sprintf(dest,"NODE %s:%s/%s",pat->Zone,pat->Net,pat->Node);
         break;
   }
}

void Print4DDestPat(struct Node4DPat *pat,char *dest)
{
   switch(pat->Type)
   {
      case PAT_PATTERN:
         sprintf(dest,"%s:%s/%s.%s",pat->Zone,pat->Net,pat->Node,pat->Point);
         break;
      case PAT_ZONE:
         sprintf(dest,"ZONE");
         break;
      case PAT_REGION:
         sprintf(dest,"REGION");
         break;
      case PAT_NET:
         sprintf(dest,"NET");
         break;
      case PAT_HUB:
         sprintf(dest,"HUB");
         break;
      case PAT_NODE:
         sprintf(dest,"NODE");
         break;
   }
}

bool Check4DPatNodelist(struct Node4DPat *pat)
{
   if(pat->Type == PAT_HUB || pat->Type == PAT_REGION)
      return(FALSE);

   return(TRUE);
}

bool Parse2DPat(char *buf, struct Node2DPat *node)
{
   uint32_t c=0,tempc=0;
   char temp[10];
   bool GotNet=FALSE;

   strcpy(node->Net,"*");
   strcpy(node->Node,"*");

   if(strcmp(buf,"*")==0)
      return(TRUE);

	for(c=0;c<strlen(buf);c++)
	{
		if(buf[c]=='/')
		{
         if(GotNet) return(FALSE);
         strcpy(node->Net,temp);
         GotNet=TRUE;
			tempc=0;
		}
		else if((buf[c]>='0' && buf[c]<='9') || buf[c]=='*' || buf[c]=='?')
		{
         if(tempc<9)
         {
            temp[tempc++]=buf[c];
            temp[tempc]=0;
         }
		}
		else return(FALSE);
	}

   strcpy(node->Node,temp);
   return(TRUE);
}

int Compare2DPat(struct Node2DPat *nodepat,uint16_t net,uint16_t node)
{
      if(NodeCompare(nodepat->Net,  net )!=0) return(1);
      if(NodeCompare(nodepat->Node, node)!=0) return(1);
      return(0);
}

/* Expand destpat */

bool iswildcard(char *str)
{
   int c;

   for(c=0;c<strlen(str);c++)
      if(str[c]=='*' || str[c]=='?') return(TRUE);

   return(FALSE);
}

void ExpandNodePat(struct Node4DPat *temproute,struct Node4D *dest,struct Node4D *sendto)
{
   long region,hub;
	struct Node4D n4d;
	   
   region=0;
   hub=0; 
   
   if(nodelistopen)
   {
		Copy4D(&n4d,dest);
		n4d.Point=0;

      if(temproute->Type == PAT_REGION) region=(*config.cfg_NodelistType->nlGetRegion)(&n4d);
      if(temproute->Type == PAT_HUB) hub=(*config.cfg_NodelistType->nlGetHub)(&n4d);

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
