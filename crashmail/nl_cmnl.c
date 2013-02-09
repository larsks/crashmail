#include "crashmail.h"
#include "cmnllib/cmnllib.h"

osFile nlfh;

bool cmnl_nlStart(char *errbuf)
{
	if(!(nlfh=cmnlOpenNL(config.cfg_Nodelist)))
   {
   	strcpy(errbuf,cmnlLastError());
      return(FALSE);
   }

	return(TRUE);
}

void cmnl_nlEnd(void)
{
	cmnlCloseNL(nlfh);	
}

bool cmnl_nlCheckNode(struct Node4D *node)
{
	struct cmnlIdx idx;

   idx.zone=node->Zone;
   idx.net=node->Net;
   idx.node=node->Node;
   idx.point=node->Point;

   if(!cmnlFindNL(nlfh,config.cfg_Nodelist,&idx,NULL,0))
		return(FALSE);
		
	return(TRUE);
}

long cmnl_nlGetHub(struct Node4D *node)
{
	struct cmnlIdx idx;

   idx.zone=node->Zone;
   idx.net=node->Net;
   idx.node=node->Node;
   idx.point=node->Point;

   if(!cmnlFindNL(nlfh,config.cfg_Nodelist,&idx,NULL,0))
		return(-1);
		
	return(idx.hub);
}

long cmnl_nlGetRegion(struct Node4D *node)
{
	struct cmnlIdx idx;

   idx.zone=node->Zone;
   idx.net=node->Net;
   idx.node=node->Node;
   idx.point=node->Point;

   if(!cmnlFindNL(nlfh,config.cfg_Nodelist,&idx,NULL,0))
		return(-1);
		
	return(idx.region);
}
