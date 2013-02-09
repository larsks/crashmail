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
      LogWrite(2,TOSSINGINFO,"%u messages exported",scan_total);
   }
}

bool ScanHandle(struct MemMessage *mm)
{
   char buf[50];

   if(mm->Area[0]==0)
   {
      Print4D(&mm->DestNode,buf);

      LogWrite(4,TOSSINGINFO,"Exporting message #%u from \"%s\" to \"%s\" at %s",
         mm->msgnum,
         mm->From,
         mm->To,
         buf);
   }
   else
   {
      LogWrite(4,TOSSINGINFO,"Exporting message #%u from \"%s\" to \"%s\" in %s",
         mm->msgnum,
         mm->From,
         mm->To,
         mm->Area);
   }

   return HandleMessage(mm);
}

bool Scan(void)
{
   struct Area *area;

   LogWrite(2,ACTIONINFO,"Scanning all areas for messages to export");

   if(!BeforeScanToss())
      return(FALSE);

   for(area=(struct Area *)config.AreaList.First;area && !ctrlc;area=area->Next)
      if(area->Messagebase && (area->AreaType == AREATYPE_ECHOMAIL || area->AreaType == AREATYPE_NETMAIL))
		{
         if(area->Messagebase->exportfunc)
			{
            if(area->AreaType == AREATYPE_NETMAIL && (config.cfg_Flags & CFG_NOEXPORTNETMAIL))
            {
               printf("Skipping area %s (NOEXPORTNETMAIL is set)\n", area->Tagname);
            }
            else
            {
               printf("Scanning area %s\n",area->Tagname);

	            if(!(*area->Messagebase->exportfunc)(area,ScanHandle))
   	         {
          	      AfterScanToss(FALSE);
            	   return(FALSE);
	            }
            }
         }
      }

   if(ctrlc)
   {
      AfterScanToss(FALSE);
      return(FALSE);
   }

   LogScanResults();
   AfterScanToss(TRUE);

   return(TRUE);
}

bool ScanList(char *file)
{
   osFile fh;
   char buf[100];
   struct Area *area;
   int res;

   LogWrite(2,ACTIONINFO,"Scanning areas in %s for messages to export",file);

   if(!(fh=osOpen(file,MODE_OLDFILE)))
   {
      LogWrite(1,USERERR,"Unable to open %s",file);
      return(FALSE);
   }

   if(!BeforeScanToss())
      return(FALSE);

   while(osFGets(fh,buf,100) && !ctrlc)
   {
      striptrail(buf);

      if(buf[0] != 0)
      {
         striptrail(buf);

         for(area=(struct Area *)config.AreaList.First;area;area=area->Next)
            if(stricmp(area->Tagname,buf)==0) break;

         if(!area)
         {
            LogWrite(1,USERERR,"Skipping area %s, area is unknown",buf);
         }
         else if(!area->scanned)
         {
            area->scanned=TRUE;

            if(area->AreaType != AREATYPE_ECHOMAIL && area->AreaType != AREATYPE_NETMAIL)
		      {
               LogWrite(1,USERERR,"Skipping area %s, not an echomail or netmail area",area->Tagname);
		      }
            else if(area->AreaType == AREATYPE_NETMAIL && (config.cfg_Flags & CFG_NOEXPORTNETMAIL))
            {
               LogWrite(1,USERERR,"Skipping area %s (NOEXPORTNETMAIL is set)", area->Tagname);
            }
            else if(!area->Messagebase)
		      {
               LogWrite(1,USERERR,"Skipping area %s, area is pass-through",area->Tagname);
		      }
		      else if(!area->Messagebase->exportfunc)
		      {
               LogWrite(1,USERERR,"Skipping area %s, scanning is not supported for this type of messagebase",area->Tagname);
		      }
		      else
		      {
               printf("Scanning area %s\n",area->Tagname);

               res=(*area->Messagebase->exportfunc)(area,ScanHandle);

               if(!res)
               {
                  AfterScanToss(FALSE);
                  osClose(fh);
                  return(FALSE);
               }
            }
         }
      }
   }

   osClose(fh);

   if(ctrlc)
   {
      AfterScanToss(FALSE);
      return(FALSE);
   }

   LogScanResults();
   AfterScanToss(TRUE);

   return(TRUE);
}

bool ScanDotJam(char *file)
{
   osFile fh;
   char buf[100];
   struct Area *area;
   int res;

   LogWrite(2,ACTIONINFO,"Scanning areas in %s for messages to export",file);

   if(!(fh=osOpen(file,MODE_OLDFILE)))
   {
      LogWrite(1,USERERR,"Unable to open %s",file);
      return(FALSE);
   }

   if(!BeforeScanToss())
      return(FALSE);

   while(osFGets(fh,buf,100) && !ctrlc)
   {
      striptrail(buf);

      if(buf[0] != 0)
      {
         striptrail(buf);

         if(strchr(buf,' '))
            *strchr(buf,' ')=0;

         for(area=(struct Area *)config.AreaList.First;area;area=area->Next)
            if(stricmp(area->Path,buf)==0) break;

         if(!area)
         {
            LogWrite(1,USERERR,"No area with path %s",buf);
         }
         else if(!area->scanned)
         {
            area->scanned=TRUE;

            if(area->AreaType != AREATYPE_ECHOMAIL && area->AreaType != AREATYPE_NETMAIL)
		      {
               LogWrite(1,USERERR,"Skipping area %s, not an echomail or netmail area",area->Tagname);
		      }
            else if(area->AreaType == AREATYPE_NETMAIL && (config.cfg_Flags & CFG_NOEXPORTNETMAIL))
            {
               LogWrite(1,USERERR,"Skipping area %s (NOEXPORTNETMAIL is set)", area->Tagname);
            }
            else if(!area->Messagebase)
		      {
               LogWrite(1,USERERR,"Skipping area %s, area is pass-through",area->Tagname);
		      }
		      else if(!area->Messagebase->exportfunc)
		      {
               LogWrite(1,USERERR,"Skipping area %s, scanning is not supported for this type of messagebase",area->Tagname);
		      }
		      else
		      {
               printf("Scanning area %s\n",area->Tagname);

               res=(*area->Messagebase->exportfunc)(area,ScanHandle);

               if(!res)
               {
                  AfterScanToss(FALSE);
                  osClose(fh);
                  return(FALSE);
               }
            }
         }
      }
   }

   osClose(fh);

   if(ctrlc)
   {
      AfterScanToss(FALSE);
      return(FALSE);
   }
   
   LogScanResults();
   AfterScanToss(TRUE);

   return(TRUE);
}

bool ScanArea(char *tagname)
{
   struct Area *area;
   int res;

   for(area=(struct Area *)config.AreaList.First;area;area=area->Next)
      if(stricmp(area->Tagname,tagname)==0) break;

   if(!area)
   {
      LogWrite(1,USERERR,"Unknown area %s",tagname);
      return(FALSE);
   }

   if(area->AreaType != AREATYPE_ECHOMAIL && area->AreaType != AREATYPE_NETMAIL)
	{
      LogWrite(1,USERERR,"You cannot scan area %s, not an echomail or netmail area",area->Tagname);
      return(FALSE);
   }
	else if(!area->Messagebase)
	{
      LogWrite(1,USERERR,"You cannot scan area %s, area is pass-through",area->Tagname);
      return(FALSE);
	}
	else if(!area->Messagebase->exportfunc)
	{
      LogWrite(1,USERERR,"You cannot scan area %s, scanning is not supported for this type of messagebase",area->Tagname);
      return(FALSE);
	}

   if(!BeforeScanToss())
      return(FALSE);

   printf("Scanning area %s\n",area->Tagname);
   res=(*area->Messagebase->exportfunc)(area,ScanHandle);

   if(!res)
   {
      AfterScanToss(FALSE);
      return(FALSE);
   }

   LogScanResults();
   AfterScanToss(TRUE);

   return(TRUE);
}

