#include "crashmail.h"

void LogScanResults(void)
{
   printf("\n");

   if(scan_total==0)
     LogWrite(2,TOSSINGINFO,"No messages exported");

   else if(scan_total==1)
      LogWrite(2,TOSSINGINFO,"1 message exported");

   else
   {
      LogWrite(2,TOSSINGINFO,"%lu messages exported",scan_total);
   }
}

bool ScanHandle(struct MemMessage *mm)
{
   uchar buf[50];
   
   if(mm->Area[0]==0)
   {
      Print4D(&mm->DestNode,buf);

      LogWrite(4,TOSSINGINFO,"Exporting message #%lu from \"%s\" to \"%s\" at %s",
         mm->msgnum,
         mm->From,
         mm->To,
         buf);
   }
   else
   {
      LogWrite(4,TOSSINGINFO,"Exporting message #%lu from \"%s\" to \"%s\" in %s",
         mm->msgnum,
         mm->From,
         mm->To,
         mm->Area);
   }

   return HandleMessage(mm);
}

bool DoScanArea(uchar *tagname)
{
   struct Area *area;

   for(area=(struct Area *)config.AreaList.First;area;area=area->Next)
      if(stricmp(area->Tagname,tagname)==0) break;

   if(area)
   {
		if(area->AreaType != AREATYPE_ECHOMAIL && area->AreaType != AREATYPE_NETMAIL)
		{
         LogWrite(1,USERERR,"You cannot scan area %s, not an echomail or netmail area",tagname);
		}
		else if(!area->Messagebase)
		{
         LogWrite(1,USERERR,"You cannot scan area %s, area is pass-through",tagname);
		}
		else if(!area->Messagebase->exportfunc)
		{
         LogWrite(1,USERERR,"You cannot scan area %s, scanning is not supported for this type of messagebase",tagname);
		}
		else
		{
         printf("Scanning area %s\n",area->Tagname);
         return (*area->Messagebase->exportfunc)(area,ScanHandle);
      }
   }
   else
   {
      LogWrite(1,USERERR,"Unknown area %s",tagname);
   }

   return(FALSE);
}

bool Scan(void)
{
   struct Area *area;

   LogWrite(2,ACTIONINFO,"Scanning areas for messages to export");

   istossing=FALSE;
   isscanning=TRUE;
   isrescanning=FALSE;

   if(!BeforeScanToss())
      return(FALSE);

   for(area=(struct Area *)config.AreaList.First;area;area=area->Next)
      if(area->Messagebase && (area->AreaType == AREATYPE_ECHOMAIL || area->AreaType == AREATYPE_NETMAIL))
		{
			if(area->Messagebase->exportfunc)
			{
	         printf("Scanning area %s\n",area->Tagname);

	         if(!(*area->Messagebase->exportfunc)(area,ScanHandle))
   	      {
      	      AfterScanToss(FALSE);
         	   return(FALSE);
	         }
			}
      }
   
   LogScanResults();
   AfterScanToss(TRUE);

   return(TRUE);
}

bool ScanList(uchar *file)
{
   osFile fh;
   uchar buf[100];

   if(!(fh=osOpen(file,MODE_OLDFILE)))
   {
      LogWrite(1,USERERR,"Unable to open %s",file);
      return(FALSE);
   }

   istossing=FALSE;
   isscanning=TRUE;
   isrescanning=FALSE;

   if(!BeforeScanToss())
      return(FALSE);

   while(osFGets(fh,buf,100))
   {
      if(strlen(buf)>2)
      {
         if(buf[strlen(buf)-1]<32)
            buf[strlen(buf)-1]=0;

         if(!DoScanArea(buf))
         {
            AfterScanToss(FALSE);
            osClose(fh);
            return(FALSE);
         }
      }
   }

   osClose(fh);

   LogScanResults();
   AfterScanToss(TRUE);
      
   return(TRUE);
}

bool ScanArea(uchar *tagname)
{
   istossing=FALSE;
   isscanning=TRUE;
   isrescanning=FALSE;

   if(!BeforeScanToss())
      return(FALSE);

   if(!DoScanArea(tagname))
   {
      AfterScanToss(FALSE);
      return(FALSE);
   }

   LogScanResults();
   AfterScanToss(TRUE);
 
   return(TRUE);
}

