#include "crashmail.h"

#define STATS_IDENTIFIER      "CST3"

struct DiskAreaStats
{
   uchar Tagname[80];
   struct Node4D Aka;

   uchar Group;
   uchar fill_to_make_even; /* Just ignore this one */

   uint32_t TotalTexts;
   uint16_t Last8Days[8];
   uint32_t Dupes;

   time_t FirstTime;
   time_t LastTime;
};

struct DiskNodeStats
{
   struct Node4D Node;
   uint32_t GotNetmails;
   uint32_t GotNetmailBytes;
   uint32_t SentNetmails;
   uint32_t SentNetmailBytes;
   uint32_t GotEchomails;
   uint32_t GotEchomailBytes;
   uint32_t SentEchomails;
   uint32_t SentEchomailBytes;
   uint32_t Dupes;
   time_t FirstTime;
};

bool WriteStats(uchar *file)
{
   struct Area *area;
   struct ConfigNode *cnode;
   struct DiskAreaStats dastat;
   struct DiskNodeStats dnstat;
   osFile fh;
   uint32_t areas,nodes;

   if(!(fh=osOpen(file,MODE_NEWFILE)))
   {
		uint32_t err=osError();
      LogWrite(1,SYSTEMERR,"Unable to open %s for writing",file);
		LogWrite(1,SYSTEMERR,"Error: %s",osErrorMsg(err));
      return(FALSE);
   }

   areas=0;
   nodes=0;

   for(area=(struct Area *)config.AreaList.First;area;area=area->Next)
		if(area->AreaType == AREATYPE_BAD || area->AreaType == AREATYPE_ECHOMAIL || area->AreaType == AREATYPE_NETMAIL)
		{
      	if(!(area->Flags & AREA_UNCONFIRMED))
	         areas++;
		}
		
   for(cnode=(struct ConfigNode *)config.CNodeList.First;cnode;cnode=cnode->Next)
      nodes++;

   if(DayStatsWritten == 0)
      DayStatsWritten = time(NULL) / (24*60*60);

   osWrite(fh,STATS_IDENTIFIER,4);
   osWrite(fh,&DayStatsWritten,sizeof(uint32_t));
   osWrite(fh,&areas,sizeof(uint32_t));

   for(area=(struct Area *)config.AreaList.First;area;area=area->Next)
   {
		if(area->AreaType == AREATYPE_BAD || area->AreaType == AREATYPE_ECHOMAIL || area->AreaType == AREATYPE_NETMAIL)
		{
      	if(!(area->Flags & AREA_UNCONFIRMED))
			{
	         strcpy(dastat.Tagname,area->Tagname);
   	      dastat.TotalTexts=area->Texts;
      	   dastat.Dupes=area->Dupes;
	         dastat.LastTime=area->LastTime;
   	      dastat.FirstTime=area->FirstTime;
      	   memcpy(&dastat.Last8Days[0],&area->Last8Days[0],sizeof(uint16_t)*8);
	         Copy4D(&dastat.Aka,&area->Aka->Node);
   	      dastat.Group=area->Group;

      	   osWrite(fh,&dastat,sizeof(struct DiskAreaStats));
			}
      }
   }

   osWrite(fh,&nodes,sizeof(uint32_t));

   for(cnode=(struct ConfigNode *)config.CNodeList.First;cnode;cnode=cnode->Next)
   {
      Copy4D(&dnstat.Node,&cnode->Node);
      dnstat.GotEchomails=cnode->GotEchomails;
      dnstat.GotEchomailBytes=cnode->GotEchomailBytes;
      dnstat.SentEchomails=cnode->SentEchomails;
      dnstat.SentEchomailBytes=cnode->SentEchomailBytes;
      dnstat.GotNetmails=cnode->GotNetmails;
      dnstat.GotNetmailBytes=cnode->GotNetmailBytes;
      dnstat.SentNetmails=cnode->SentNetmails;
      dnstat.SentNetmailBytes=cnode->SentNetmailBytes;
      dnstat.Dupes=cnode->Dupes;
      dnstat.FirstTime=cnode->FirstTime;

      osWrite(fh,&dnstat,sizeof(struct DiskNodeStats));
   }

   osClose(fh);

   return(TRUE);
}

bool ReadStats(uchar *file)
{
   struct Area *area;
   struct ConfigNode *cnode;
   struct DiskAreaStats dastat;
   struct DiskNodeStats dnstat;
   uint32_t c,num;
   osFile fh;
   uchar buf[5];

   if(!(fh=osOpen(file,MODE_OLDFILE)))
      return(TRUE); /* No reason for exiting */

   osRead(fh,buf,4);
   buf[4]=0;

   if(strcmp(buf,STATS_IDENTIFIER)!=0)
   {
      LogWrite(1,SYSTEMERR,"Unknown format of stats file %s, exiting...",file);
      osClose(fh);
      return(FALSE);
   }

   osRead(fh,&DayStatsWritten,sizeof(uint32_t));

   osRead(fh,&num,sizeof(uint32_t));
   c=0;

   while(c<num && osRead(fh,&dastat,sizeof(struct DiskAreaStats))==sizeof(struct DiskAreaStats))
   {
      for(area=(struct Area *)config.AreaList.First;area;area=area->Next)
         if(stricmp(area->Tagname,dastat.Tagname)==0) break;

      if(area)
      {
         area->Texts=dastat.TotalTexts;
         area->Dupes=dastat.Dupes;
         area->FirstTime=dastat.FirstTime;
         area->LastTime=dastat.LastTime;
         memcpy(&area->Last8Days[0],&dastat.Last8Days[0],sizeof(uint16_t)*8);
      }

      c++;
   }

   osRead(fh,&num,sizeof(uint32_t));
   c=0;

   while(c<num && osRead(fh,&dnstat,sizeof(struct DiskNodeStats))==sizeof(struct DiskNodeStats))
   {
      for(cnode=(struct ConfigNode *)config.CNodeList.First;cnode;cnode=cnode->Next)
         if(Compare4D(&dnstat.Node,&cnode->Node)==0) break;

      if(cnode)
      {
         cnode->GotEchomails=dnstat.GotEchomails;
         cnode->GotEchomailBytes=dnstat.GotEchomailBytes;
         cnode->SentEchomails=dnstat.SentEchomails;
         cnode->SentEchomailBytes=dnstat.SentEchomailBytes;
         cnode->GotNetmails=dnstat.GotNetmails;
         cnode->GotNetmailBytes=dnstat.GotNetmailBytes;
         cnode->SentNetmails=dnstat.SentNetmails;
         cnode->SentNetmailBytes=dnstat.SentNetmailBytes;
         cnode->Dupes=dnstat.Dupes;

         cnode->FirstTime=dnstat.FirstTime;
      }

      c++;
   }

   osClose(fh);

   return(TRUE);
}
