#include "crashmail.h"

bool Compare(uchar *str,uchar *recog)
{
   uint32_t c,d;
   uchar comp;
   uchar buf[5];

   d=0;

   for(c=0;d<strlen(recog);c++)
   {
      if(recog[d]=='$')
      {
         strncpy(buf,&recog[d+1],2);
         buf[2]=0;
         comp=hextodec(buf);
         if(str[c]!=comp) return(FALSE);
         d+=3;
      }
      else
      {
         if(str[c]!=recog[d] && recog[d]!='?') return(FALSE);
         d++;
      }
   }

   return(TRUE);
}

struct Packer *DetectPacker(uchar *file)
{
   osFile fh;
   uchar buf[40];
   struct Packer *tmppacker;

   if(!(fh=osOpen(file,MODE_OLDFILE)))
   {
		uint32_t err=osError();
      LogWrite(1,SYSTEMERR,"Unable to open %s",file);
		LogWrite(1,SYSTEMERR,"Error: %s",osErrorMsg(err));
      return(NULL);
   }

   osRead(fh,buf,40);
   osClose(fh);

   for(tmppacker=(struct Packer *)config.PackerList.First;tmppacker;tmppacker=tmppacker->Next)
      if(Compare(buf,tmppacker->Recog)) return(tmppacker);

   return(NULL);
}

void LogTossResults(void)
{
   struct Area *area;

   printf("\n");

   for(area=(struct Area *)config.AreaList.First;area;area=area->Next)
   {
      if(ctrlc)
         return;

      if(area->NewDupes)
         LogWrite(3,TOSSINGINFO,"Area %s -- %lu messages (%lu dupes)",area->Tagname,area->NewTexts,area->NewDupes);

      else if(area->NewTexts)
         LogWrite(3,TOSSINGINFO,"Area %s -- %lu messages",area->Tagname,area->NewTexts);
   }

   printf("\n");

   LogWrite(1,TOSSINGINFO,"   Total ->     Read messages: %6lu     Written messages: %6lu",toss_read,toss_written);
   LogWrite(1,TOSSINGINFO,"Imported -> Imported messages: %6lu      Routed netmails: %6lu",toss_import,toss_route);
   LogWrite(1,TOSSINGINFO,"     Bad ->      Bad messages: %6lu   Duplicate messages: %6lu",toss_bad,toss_dupes);

   printf("\n");
}

bool TossBundle(uchar *file,struct osFileEntry *fe)
{
   struct Packer *tmppacker;
   uchar buf[200],buf2[100];
   struct jbList FEList;
   struct osFileEntry *pktfe,*safedel;
   int arcres;

   if(fe->Size == 0)
   {
      LogWrite(1,TOSSINGINFO,"Deleting zero length bundle %s",file);
      osDelete(file);
      return(TRUE);
   }

   if(!(tmppacker=(struct Packer *)DetectPacker(file)))
   {
      LogWrite(1,TOSSINGINFO,"Failed to recognize archive type of %s",file);
      BadFile(file,"Unknown packer");
      return(TRUE);
   }

   printf("\n");
   LogWrite(2,TOSSINGINFO,"Unarchiving %s using %s",file,tmppacker->Name);
   printf("\n");

   ExpandPacker(tmppacker->Unpacker,buf2,100,file,"");

   arcres=osChDirExecute(config.cfg_TempDir,buf2);
   printf("\n");
   
   if(arcres == -1)
   {
      LogWrite(1,SYSTEMERR,"Unable to find temp directory \"%s\"",config.cfg_TempDir);
      return(FALSE);
   }

   if(arcres!=0)
   {
      LogWrite(1,SYSTEMERR,"Unarchiving failed: %lu",arcres);
      sprintf(buf2,"Unarchiving with %s failed: %u",tmppacker->Name,arcres);
      BadFile(file,buf2);
   }

   if(!osReadDir(config.cfg_TempDir,&FEList,IsPkt))
   {
		uint32_t err=osError();
      LogWrite(1,SYSTEMERR,"Failed to read directory \"%s\"",config.cfg_TempDir);
		LogWrite(1,SYSTEMERR,"Error: %s",osErrorMsg(err));
      return(FALSE);
   }

   if(!SortFEList(&FEList))
   {
      jbFreeList(&FEList);
      return(FALSE);
   }

   for(pktfe=(struct osFileEntry *)FEList.First;pktfe && !ctrlc;pktfe=pktfe->Next)
   {
      MakeFullPath(config.cfg_TempDir,pktfe->Name,buf,200);

      /* If you have your tempdir in Inbound, this might be an unpacked
         mailpacket that has been processed already */

      for(safedel=(struct osFileEntry *)DeleteList.First;safedel;safedel=safedel->Next)
         if(strcmp(safedel->Name,buf)==0) break;

      if(!safedel)
      {
         if(!ReadPkt(buf,pktfe,TRUE,HandleMessage))
         {
            jbFreeList(&FEList);
            return(FALSE);
         }

         osDelete(buf);
      }
   }

   jbFreeList(&FEList);

   if(ctrlc)
      return(FALSE);

   return(TRUE);
}

bool TossDir(uchar *dir)
{
   struct osFileEntry *fe;
   struct jbList PktFEList;
   struct jbList ArcFEList;
   uchar buf[200];

   LogWrite(3,ACTIONINFO,"Tossing files in %s...",dir);

   if(!BeforeScanToss())
      return(FALSE);

   /* Notify about old bad files */

   if(!osReadDir(dir,&PktFEList,IsBad))
   {
		uint32_t err=osError();
      LogWrite(1,SYSTEMERR,"Failed to read directory \"%s\"",dir);
		LogWrite(1,SYSTEMERR,"Error: %s",osErrorMsg(err));
      AfterScanToss(FALSE);
      return(FALSE);
   }

   for(fe=(struct osFileEntry *)PktFEList.First;fe;fe=fe->Next)
      LogWrite(1,TOSSINGINFO,"Old bad file found in inbound: %s",fe->Name);

   jbFreeList(&PktFEList);

   /* Search for pkt files */

   if(!osReadDir(dir,&PktFEList,IsPkt))
   {
		uint32_t err=osError();
      LogWrite(1,SYSTEMERR,"Failed to read directory \"%s\"",dir);
		LogWrite(1,SYSTEMERR,"Error: %s",osErrorMsg(err));
      AfterScanToss(FALSE);
      return(FALSE);
   }

   /* Search for bundles */

   if(!osReadDir(dir,&ArcFEList,IsArc))
   {
		uint32_t err=osError();
      LogWrite(1,SYSTEMERR,"Failed to read directory \"%s\"",dir);
		LogWrite(1,SYSTEMERR,"Error: %s",osErrorMsg(err));
      jbFreeList(&PktFEList);
      AfterScanToss(FALSE);
      return(FALSE);
   }

   SortFEList(&PktFEList);
   SortFEList(&ArcFEList);

   if(nomem)
   {
      jbFreeList(&PktFEList);
      jbFreeList(&ArcFEList);
      AfterScanToss(FALSE);
      return(FALSE);
   }

   /* Process pkt files */

   for(fe=(struct osFileEntry *)PktFEList.First;fe && !ctrlc;fe=fe->Next)
   {
      MakeFullPath(dir,fe->Name,buf,200);

      if(!ReadPkt(buf,fe,FALSE,HandleMessage))
      {
         jbFreeList(&PktFEList);
         jbFreeList(&ArcFEList);
         AfterScanToss(FALSE);
         return(FALSE);
      }

      SafeDelete(buf);
   }

   for(fe=(struct osFileEntry *)ArcFEList.First;fe && !ctrlc;fe=fe->Next)
   {
      MakeFullPath(dir,fe->Name,buf,200);

      if(!TossBundle(buf,fe))
      {
         jbFreeList(&PktFEList);
         jbFreeList(&ArcFEList);
         AfterScanToss(FALSE);
         return(FALSE);
      }

      SafeDelete(buf);
   }

   jbFreeList(&PktFEList);
   jbFreeList(&ArcFEList);

   if(ctrlc)
   {
      AfterScanToss(FALSE);
      return(FALSE);
   }

   LogTossResults();
   AfterScanToss(TRUE);

   return(TRUE);
}

bool TossFile(uchar *file)
{
   struct osFileEntry *fe;

   LogWrite(3,ACTIONINFO,"Tossing file %s...",file);

   if(!(fe=osGetFileEntry(file)))
   {
		uint32_t err=osError();
      LogWrite(1,SYSTEMERR,"Failed to read file \"%s\"",file);
		LogWrite(1,SYSTEMERR,"Error: %s",osErrorMsg(err));
      return(FALSE);
   }

   if(!BeforeScanToss())
   {
      osFree(fe);
      return(FALSE);
   }

   if(IsPkt(fe->Name))
   {
      if(!ReadPkt(file,fe,FALSE,HandleMessage))
      {
         osFree(fe);
         AfterScanToss(FALSE);
         return(FALSE);
      }

      SafeDelete(file);
   }
   else
   {
      if(!TossBundle(file,fe))
      {
         osFree(fe);
         AfterScanToss(FALSE);
         return(FALSE);
      }

      SafeDelete(file);
   }

   LogTossResults();
   AfterScanToss(TRUE);

   return(TRUE);
}

