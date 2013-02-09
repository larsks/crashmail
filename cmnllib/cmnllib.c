#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <oslib/os.h>
#include <oslib/osfile.h>
#include <shared/types.h>
#include <shared/path.h>

#include "cmnllib.h"

struct idx
{
	uint16_t zone,net,node,point,region,hub;
	uint32_t offset;
};
                  
uint32_t cmnlerr;						                                               

char *cmnlerrstr[] = 
{
	"",
	"Failed to open nodelist index",
	"Unknown format of nodelist index",
	"Unexpected end of file",
	"Node not found",
	"Failed to open nodelist"
};	

uint16_t cmnlgetuword(uint8_t *buf,uint32_t offset)
{
   return (uint16_t)(buf[offset]+256*buf[offset+1]);
}

uint32_t cmnlgetlong(uint8_t *buf,uint32_t offset)
{
   return (long) buf[offset]+
				 	  buf[offset+1]*256+
					  buf[offset+2]*256*256+
					  buf[offset+3]*256*256*256;
}

osFile cmnlOpenNL(char *dir)
{
	osFile fh;
	char buf[200];

	MakeFullPath(dir,"cmnodelist.index",buf,200);

	if(!(fh=osOpen(buf,MODE_OLDFILE)))
	{
		cmnlerr=CMNLERR_NO_INDEX;
		return(NULL);
	}	

	osRead(fh,buf,4);
	buf[4]=0;
	
	if(strcmp(buf,"CNL1")!=0)
	{
		osClose(fh);
		cmnlerr=CMNLERR_WRONG_TYPE;
		return(NULL);
	}
	
	return(fh);
}

void cmnlCloseNL(osFile nl)
{
	osClose(nl);
}

bool cmnlFindNL(osFile nl,char *dir,struct cmnlIdx *cmnlidx,char *line,uint32_t len)
{
	char buf[200];
	char nlname[100];
	struct idx idx;
	bool found;
	osFile fh;
	uint8_t binbuf[16];

	osSeek(nl,4,OFFSET_BEGINNING);

	found=FALSE;

	while(!found)
	{
		if(osRead(nl,nlname,100)!=100)
		{
			cmnlerr=CMNLERR_NODE_NOT_FOUND;
			return(FALSE);
		}

		idx.offset=0;

		while(!found && idx.offset != 0xffffffff)
		{			

			if(osRead(nl,binbuf,sizeof(binbuf)) != sizeof(binbuf))
			{
				cmnlerr=CMNLERR_UNEXPECTED_EOF;
				return(FALSE);
			}

			idx.zone=cmnlgetuword(binbuf,0);
			idx.net=cmnlgetuword(binbuf,2);
			idx.node=cmnlgetuword(binbuf,4);
			idx.point=cmnlgetuword(binbuf,6);
			idx.region=cmnlgetuword(binbuf,8);
			idx.hub=cmnlgetuword(binbuf,10);
			idx.offset=cmnlgetlong(binbuf,12);

			found=TRUE;

			if(cmnlidx->zone  != idx.zone)  found=FALSE;
			if(cmnlidx->net   != idx.net)   found=FALSE;
			if(cmnlidx->node  != idx.node)  found=FALSE;
			if(cmnlidx->point != idx.point) found=FALSE;
		}
	}

	cmnlidx->region=idx.region;
	cmnlidx->hub=idx.hub;

	if(!line)
	{
		return(TRUE);
	}

	MakeFullPath(dir,nlname,buf,200);

	if(!(fh=osOpen(buf,MODE_OLDFILE)))
	{
		cmnlerr=CMNLERR_NO_NODELIST;
		return(FALSE);
	}

	osSeek(fh,idx.offset,OFFSET_BEGINNING);
	osFGets(fh,line,len);	
	osClose(fh);

	return(TRUE);	
}

char *cmnlLastError(void)
{
	return cmnlerrstr[cmnlerr];
}
