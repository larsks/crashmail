#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "node4d.h"

bool Parse4DTemplate(char *buf, struct Node4D *node,struct Node4D *tpl)
{
   uint32_t c,val;
   bool GotZone,GotNet,GotNode,GotVal;

	GotZone=FALSE;
	GotNet=FALSE;
	GotNode=FALSE;
	
	GotVal=FALSE;

	Copy4D(node,tpl);

	for(c=0;c<strlen(buf);c++)
	{
		if(buf[c]==':')
		{
         if(GotZone || GotNet || GotNode) return(FALSE);				
			if(GotVal) node->Zone=val;

         GotZone=TRUE;
			GotVal=FALSE;
	   }
		else if(buf[c]=='/')
		{
         if(GotNet || GotNode) return(FALSE);
			if(GotVal) node->Net=val;

         GotNet=TRUE;
			GotVal=FALSE;
		}
		else if(buf[c]=='.')
		{
         if(GotNode) return(FALSE);
         if(GotVal) node->Node=val;

         GotNode=TRUE;
			GotVal=FALSE;
		}
		else if(buf[c]>='0' && buf[c]<='9')
		{
			if(!GotVal)
			{
				val=0;
				GotVal=TRUE;
			}

         val*=10;
         val+=buf[c]-'0';
		}
		else return(FALSE);
	}

	if(GotVal)
	{
	   if(GotZone && !GotNet)  node->Net=val;
   	else if(GotNode)        node->Point=val;
	   else                    node->Node=val;
	}
	
   return(TRUE);
}

bool Parse4D(char *buf, struct Node4D *node)
{
	struct Node4D tpl4d = { 0,0,0,0 };
	
	return Parse4DTemplate(buf,node,&tpl4d);
}

void Copy4D(struct Node4D *node1,struct Node4D *node2)
{
   node1->Zone=node2->Zone;
   node1->Net=node2->Net;
   node1->Node=node2->Node;
   node1->Point=node2->Point;
}

int Compare4D(struct Node4D *node1,struct Node4D *node2)
{
   if(node1->Zone!=0 && node2->Zone!=0)
   {
      if(node1->Zone > node2->Zone) return(1);
      if(node1->Zone < node2->Zone) return(-1);
   }

   if(node1->Net  > node2->Net) return(1);
   if(node1->Net  < node2->Net) return(-1);

   if(node1->Node > node2->Node) return(1);
   if(node1->Node < node2->Node) return(-1);

   if(node1->Point > node2->Point) return(1);
   if(node1->Point < node2->Point) return(-1);

   return(0);
}

void Print4D(struct Node4D *n4d,char *dest)
{
   if(n4d->Point)
      sprintf(dest,"%u:%u/%u.%u",n4d->Zone,
                                     n4d->Net,
                                     n4d->Node,
                                     n4d->Point);

   else
      sprintf(dest,"%u:%u/%u",n4d->Zone,
                                 n4d->Net,
                                 n4d->Node);
}


